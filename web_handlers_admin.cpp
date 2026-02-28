/*
 * web_handlers_admin.cpp - Monitor & Service Control API Routes
 *
 * Extracted from system_webserver.cpp to keep that file manageable.
 * All routes registered here use the global `server` object (extern WebServer server).
 *
 * Routes:
 *   /api/status          - system status JSON
 *   /api/logs            - log messages JSON
 *   /api/logs/clear      - clear log buffer
 *   /api/factory-reset   - erase NVS mmdvm namespace + reboot
 *   /api/restart-mmdvm   - stop all mode tasks, restart enabled ones
 *   /api/restart-mqtt    - restart MQTT task
 *   /api/restart-services - restart all mode tasks + MQTT
 *   /api/mode-toggle     - enable/disable any radio mode
 *   /api/reboot          - graceful reboot
 *   /api/dmr-activity    - current DMR call + last 15 call history (JSON)
 *   /api/mode-status     - enabled flags + runtime connection state per mode (JSON)
 *   /api/service-status  - enabled flags + runtime status for system services (JSON)
 *
 * Config export/import → web_handlers_config.cpp
 * NVS viewer/repair    → web_handlers_nvs.cpp
 */

#include "system/web_handlers_admin.h"
#include "system/system_webserver.h"   // extern WebServer server
#include "system/system_logger.h"      // addLogMessage(), getLogMessagesJSON()
#include "system/system_oled.h"        // getDmrActivityJson(), oledTaskHandle
#include "include/config.h"
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFi.h>

// Forward declaration only — the page headers define functions in the header body,
// so including them in multiple .cpp files causes duplicate symbol errors at link time.
extern String getSystemStatusAPIJSON();

// Runtime mode flags (defined in esp32-rtos-mmdvm.ino)
extern bool modeDmrEnabled;
extern bool modeDstarEnabled;
extern bool modeYsfEnabled;
extern bool modeP25Enabled;
extern bool modeNxdnEnabled;
extern bool modePocsagEnabled;
extern bool dapnetEnabled;
extern bool mqttEnabled;
extern bool mqttConnected;
extern unsigned long lastMqttAttempt;
extern void saveSettings();

// Runtime connection status (defined in mmdvm task files)
extern volatile bool dmrLoggedIn;
extern volatile bool dapnetLoggedIn;

// Service status variables
extern bool ethEnabled;
extern volatile bool ethConnected;
extern bool wgEnabled;
extern bool wireguardConnected;
extern bool oledEnabled;
extern bool ntpEnabled;
extern volatile bool ntpSynced;
extern bool mdnsEnabled;

// API endpoint: Get logs as JSON
void handleApiLogs()
{
  String json = getLogMessagesJSON();
  server.send(200, "application/json", json);
}

// API endpoint: Clear log buffer
void handleApiLogsClear()
{
  // Clear the log buffer
  extern int logBufferCount;
  extern int logBufferIndex;
  extern SemaphoreHandle_t logMutex;

  if (logMutex != NULL && xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE)
  {
    logBufferCount = 0;
    logBufferIndex = 0;
    xSemaphoreGive(logMutex);
  }

  server.send(200, "text/plain", "OK");
  addLogMessage("[WebServer] Log buffer cleared");
}

void registerAdminRoutes()
{
  // API status and log endpoints
  server.on("/api/status", HTTP_GET, []()
            { server.send(200, "application/json", getSystemStatusAPIJSON()); });
  server.on("/api/logs", HTTP_GET, handleApiLogs);
  server.on("/api/logs/clear", HTTP_POST, handleApiLogsClear);

  // Factory Reset endpoint - clears all NVS preferences and reboots
  server.on("/api/factory-reset", HTTP_POST, []()
            {
    extern Preferences preferences;
    addLogMessage("[System] Factory reset requested via web interface");
    preferences.begin("mmdvm", false);
    preferences.clear();
    preferences.end();
    server.send(200, "text/plain", "Factory reset complete. Rebooting...");
    addLogMessage("[System] NVS cleared - rebooting with defaults");
    delay(500);
    ESP.restart(); });

  // Restart MMDVM - stop all 6 mode tasks, restart only enabled ones
  server.on("/api/restart-mmdvm", HTTP_POST, []()
            {
    extern TaskHandle_t dmrTaskHandle, dstarTaskHandle, ysfTaskHandle;
    extern TaskHandle_t p25TaskHandle, nxdnTaskHandle, pocsagTaskHandle;
    extern void initDmrTask(), initDstarTask(), initYsfTask();
    extern void initP25Task(), initNxdnTask(), initPocsagTask();

    // Stop all running mode tasks
    if (dmrTaskHandle != NULL) {
      vTaskDelete(dmrTaskHandle);
      dmrTaskHandle = NULL;
    }
    if (dstarTaskHandle != NULL) {
      vTaskDelete(dstarTaskHandle);
      dstarTaskHandle = NULL;
    }
    if (ysfTaskHandle != NULL) {
      vTaskDelete(ysfTaskHandle);
      ysfTaskHandle = NULL;
    }
    if (p25TaskHandle != NULL) {
      vTaskDelete(p25TaskHandle);
      p25TaskHandle = NULL;
    }
    if (nxdnTaskHandle != NULL) {
      vTaskDelete(nxdnTaskHandle);
      nxdnTaskHandle = NULL;
    }
    if (pocsagTaskHandle != NULL) {
      vTaskDelete(pocsagTaskHandle);
      pocsagTaskHandle = NULL;
    }
    addLogMessage("[Admin] All MMDVM mode tasks stopped");

    // Restart only enabled modes
    int started = 0;
    if (modeDmrEnabled) {
      initDmrTask();
      started++;
    }
    if (modeDstarEnabled) {
      initDstarTask();
      started++;
    }
    if (modeYsfEnabled) {
      initYsfTask();
      started++;
    }
    if (modeP25Enabled) {
      initP25Task();
      started++;
    }
    if (modeNxdnEnabled) {
      initNxdnTask();
      started++;
    }
    if (modePocsagEnabled) {
      initPocsagTask();
      started++;
    }

    addLogMessage("[Admin] MMDVM restarted: " + String(started) + " mode(s) enabled");
    server.send(200, "text/plain", "MMDVM restarted: " + String(started) + " mode(s) started"); });

  // Restart MQTT task (only if enabled)
  server.on("/api/restart-mqtt", HTTP_POST, []()
            {
    extern TaskHandle_t mqttTaskHandle;
    extern void initMqttTask();
    extern PubSubClient mqttClient;
    extern bool mqttConnected;
    extern unsigned long lastMqttAttempt;

    // Always stop if running
    if (mqttTaskHandle != NULL) {
      vTaskDelete(mqttTaskHandle);
      mqttTaskHandle = NULL;
      mqttClient.disconnect();
      mqttConnected = false;
      addLogMessage("[Admin] MQTT task stopped");
    }

    if (mqttEnabled) {
      lastMqttAttempt = 0;
      initMqttTask();
      addLogMessage("[Admin] MQTT task restarted");
      server.send(200, "text/plain", "MQTT service restarted");
    } else {
      server.send(200, "text/plain", "MQTT is disabled - task not started");
    } });

  // Restart all services (MMDVM modes + MQTT)
  server.on("/api/restart-services", HTTP_POST, []()
            {
    extern TaskHandle_t dmrTaskHandle, dstarTaskHandle, ysfTaskHandle;
    extern TaskHandle_t p25TaskHandle, nxdnTaskHandle, pocsagTaskHandle;
    extern TaskHandle_t mqttTaskHandle;
    extern void initDmrTask(), initDstarTask(), initYsfTask();
    extern void initP25Task(), initNxdnTask(), initPocsagTask();
    extern void initMqttTask();
    extern PubSubClient mqttClient;
    extern bool mqttConnected;
    extern unsigned long lastMqttAttempt;

    // Stop all mode tasks
    if (dmrTaskHandle != NULL) {
      vTaskDelete(dmrTaskHandle);
      dmrTaskHandle = NULL;
    }
    if (dstarTaskHandle != NULL) {
      vTaskDelete(dstarTaskHandle);
      dstarTaskHandle = NULL;
    }
    if (ysfTaskHandle != NULL) {
      vTaskDelete(ysfTaskHandle);
      ysfTaskHandle = NULL;
    }
    if (p25TaskHandle != NULL) {
      vTaskDelete(p25TaskHandle);
      p25TaskHandle = NULL;
    }
    if (nxdnTaskHandle != NULL) {
      vTaskDelete(nxdnTaskHandle);
      nxdnTaskHandle = NULL;
    }
    if (pocsagTaskHandle != NULL) {
      vTaskDelete(pocsagTaskHandle);
      pocsagTaskHandle = NULL;
    }

    // Stop MQTT
    if (mqttTaskHandle != NULL) {
      vTaskDelete(mqttTaskHandle);
      mqttTaskHandle = NULL;
      mqttClient.disconnect();
      mqttConnected = false;
    }
    addLogMessage("[Admin] All services stopped");

    // Restart only enabled modes
    int started = 0;
    if (modeDmrEnabled) {
      initDmrTask();
      started++;
    }
    if (modeDstarEnabled) {
      initDstarTask();
      started++;
    }
    if (modeYsfEnabled) {
      initYsfTask();
      started++;
    }
    if (modeP25Enabled) {
      initP25Task();
      started++;
    }
    if (modeNxdnEnabled) {
      initNxdnTask();
      started++;
    }
    if (modePocsagEnabled) {
      initPocsagTask();
      started++;
    }

    // Restart MQTT only if enabled
    if (mqttEnabled) {
      lastMqttAttempt = 0;
      initMqttTask();
    }

    addLogMessage("[Admin] Services restarted: " + String(started) + " mode(s)" + String(mqttEnabled ? " + MQTT" : ""));
    server.send(200, "text/plain", "Services restarted: " + String(started) + " mode(s)" + String(mqttEnabled ? " + MQTT" : "")); });

  // Mode toggle - enable/disable any radio mode
  server.on("/api/mode-toggle", HTTP_POST, []()
            {
    if (!server.hasArg("mode") || !server.hasArg("enable")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }

    String mode = server.arg("mode");
    bool enable = server.arg("enable") == "true";

    if (mode == "dmr") modeDmrEnabled = enable;
    else if (mode == "dstar") modeDstarEnabled = enable;
    else if (mode == "ysf") modeYsfEnabled = enable;
    else if (mode == "p25") modeP25Enabled = enable;
    else if (mode == "nxdn") modeNxdnEnabled = enable;
    else if (mode == "pocsag") modePocsagEnabled = enable;
    else if (mode == "dapnet") dapnetEnabled = enable;
    else {
      server.send(400, "text/plain", "ERROR: Unknown mode");
      return;
    }

    saveSettings();

    String action = enable ? "enabled" : "disabled";
    mode.toUpperCase();
    addLogMessage("[Settings] " + mode + " mode " + action);
    server.send(200, "text/plain", mode + " mode " + action + ". Reboot required for changes to take effect."); });

  // Reboot endpoint
  server.on("/api/reboot", HTTP_POST, []()
            {
    server.send(200, "text/plain", "Rebooting...");
    addLogMessage("[System] Reboot requested via web interface");
    delay(500);
    ESP.restart(); });

  // DMR activity: current call + last 15 history
  server.on("/api/dmr-activity", HTTP_GET, []()
            {
    server.send(200, "application/json", getDmrActivityJson()); });

  // Service status: system services enabled + runtime state
  server.on("/api/service-status", HTTP_GET, []()
            {
    bool wifiConn = (WiFi.status() == WL_CONNECTED);
    bool oledRunning = (oledTaskHandle != NULL);
    String json = "[";
    json += "{\"id\":\"wifi\",\"label\":\"WiFi\",\"enabled\":true,\"connected\":" + String(wifiConn ? "true" : "false") + "},";
    json += "{\"id\":\"ethernet\",\"label\":\"Ethernet\",\"enabled\":" + String(ethEnabled ? "true" : "false") + ",\"connected\":" + String(ethConnected ? "true" : "false") + "},";
    json += "{\"id\":\"mqtt\",\"label\":\"MQTT\",\"enabled\":" + String(mqttEnabled ? "true" : "false") + ",\"connected\":" + String(mqttConnected ? "true" : "false") + "},";
    json += "{\"id\":\"ntp\",\"label\":\"NTP\",\"enabled\":" + String(ntpEnabled ? "true" : "false") + ",\"connected\":" + String(ntpSynced ? "true" : "false") + "},";
    json += "{\"id\":\"wireguard\",\"label\":\"WireGuard\",\"enabled\":" + String(wgEnabled ? "true" : "false") + ",\"connected\":" + String(wireguardConnected ? "true" : "false") + "},";
    json += "{\"id\":\"oled\",\"label\":\"OLED\",\"enabled\":" + String(oledEnabled ? "true" : "false") + "},";
    json += "{\"id\":\"pocsag\",\"label\":\"POCSAG\",\"enabled\":" + String(modePocsagEnabled ? "true" : "false") + "},";
    json += "{\"id\":\"mdns\",\"label\":\"mDNS\",\"enabled\":" + String(mdnsEnabled ? "true" : "false") + "}";
    json += "]";
    server.send(200, "application/json", json); });

  // Mode status: enabled flags + runtime connection state
  server.on("/api/mode-status", HTTP_GET, []()
            {
    String json = "[";
    json += "{\"id\":\"dmr\",\"label\":\"DMR\",\"enabled\":" + String(modeDmrEnabled ? "true" : "false") + ",\"connected\":" + String(dmrLoggedIn ? "true" : "false") + "},";
    json += "{\"id\":\"dstar\",\"label\":\"D-STAR\",\"enabled\":" + String(modeDstarEnabled ? "true" : "false") + "},";
    json += "{\"id\":\"ysf\",\"label\":\"YSF\",\"enabled\":" + String(modeYsfEnabled ? "true" : "false") + "},";
    json += "{\"id\":\"p25\",\"label\":\"P25\",\"enabled\":" + String(modeP25Enabled ? "true" : "false") + "},";
    json += "{\"id\":\"nxdn\",\"label\":\"NXDN\",\"enabled\":" + String(modeNxdnEnabled ? "true" : "false") + "},";
    json += "{\"id\":\"dapnet\",\"label\":\"DAPNET\",\"enabled\":" + String(dapnetEnabled ? "true" : "false") + ",\"connected\":" + String(dapnetLoggedIn ? "true" : "false") + "}";
    json += "]";
    server.send(200, "application/json", json); });
}
