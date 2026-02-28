/*
 * ArduinoOTA System Module Implementation
 * Allows firmware upload from Arduino IDE over network
 */

#include "system/system_arduinoota.h"
#include "system/system_logger.h"
#include "system/system_eth.h"
#include <WiFi.h>
#include "system/system_mqtt.h"

extern String mqttArduinoOtaTaskTopic; // MQTT topic for OTA task status updates

TaskHandle_t arduinoOtaTaskHandle = NULL;

// Progress tracking for OLED display
volatile bool arduinoOtaUpdateActive = false;
volatile int arduinoOtaProgress = 0;
volatile unsigned long arduinoOtaBytesWritten = 0;
volatile unsigned long arduinoOtaBytesTotal = 0;

void initArduinoOtaTask()
{
  BaseType_t result = xTaskCreatePinnedToCore(
      arduinoOtaTask,
      "OTA Task",
      ARDUINO_OTA_TASK_STACK,
      NULL,
      ARDUINO_OTA_TASK_PRIORITY,
      &arduinoOtaTaskHandle,
      0 // Run on core 0
  );
  if (result != pdPASS)
    log_e("[ArduinoOTA] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());
}

/*
 * TASK: ArduinoOTA Handler
 * Priority: Low - Development feature
 *
 * Enables firmware upload from Arduino IDE over WiFi/Ethernet.
 * Device will appear as network port in Arduino IDE.
 */
void arduinoOtaTask(void *parameter)
{
  addLogMessage("[ArduinoOTA] Task started");
  publishMqtt(mqttArduinoOtaTaskTopic.c_str(), "{\"event\":\"started\"}");

  // Wait for network (WiFi OR Ethernet)
  addLogMessage("[ArduinoOTA] Waiting for network...");
  publishMqtt(mqttArduinoOtaTaskTopic.c_str(), "{\"event\":\"waiting_for_network\"}");
  while (WiFi.status() != WL_CONNECTED && !ethConnected)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  addLogMessage("[ArduinoOTA] Network available");
  publishMqtt(mqttArduinoOtaTaskTopic.c_str(), "{\"event\":\"network_ready\"}");

  // Get settings from NVS
  extern String mdnsHostname;
  extern String arduinoOtaPassword;
  extern int arduinoOtaPort;

  // Set hostname (uses mDNS hostname from NVS)
  ArduinoOTA.setHostname(mdnsHostname.c_str());

  // Set port from NVS
  ArduinoOTA.setPort(arduinoOtaPort);

  // Set password if configured (check at runtime for non-empty string)
  if (arduinoOtaPassword.length() > 0)
  {
    ArduinoOTA.setPassword(arduinoOtaPassword.c_str());
    addLogMessage("[ArduinoOTA] Password protection enabled");
    publishMqtt(mqttArduinoOtaTaskTopic.c_str(), "{\"event\":\"password_enabled\"}");
  }

  // Setup callbacks
  ArduinoOTA.onStart([]()
                     {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    addLogMessage("[ArduinoOTA] Start updating " + type);
    publishMqtt(mqttArduinoOtaTaskTopic.c_str(), "{\"event\":\"update_start\",\"type\":\"" + type + "\"}");
    arduinoOtaUpdateActive = true;
    arduinoOtaProgress = 0;
    arduinoOtaBytesWritten = 0;
    arduinoOtaBytesTotal = 0; });

  ArduinoOTA.onEnd([]()
                   {
    addLogMessage("[ArduinoOTA] Update complete!");
    publishMqtt(mqttArduinoOtaTaskTopic.c_str(), "{\"event\":\"update_complete\"}");
    arduinoOtaProgress = 100;
    extern void displayRebootMessage();
    displayRebootMessage();
    arduinoOtaUpdateActive = false; });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        {
    // Update progress for OLED display
    arduinoOtaBytesWritten = progress;
    arduinoOtaBytesTotal = total;
    arduinoOtaProgress = (total > 0) ? (progress * 100 / total) : 0;

    // Log every 10%
    static int lastLoggedPercent = -1;
    if (arduinoOtaProgress / 10 != lastLoggedPercent / 10) {
      addLogMessage("[ArduinoOTA] Progress: " + String(arduinoOtaProgress) + "%");
      //publishMqtt(mqttArduinoOtaTaskTopic.c_str(), "{\"event\":\"progress\",\"pct\":" + String(arduinoOtaProgress) + "}");
      lastLoggedPercent = arduinoOtaProgress;
    } });

  ArduinoOTA.onError([](ota_error_t error)
                     {
    arduinoOtaUpdateActive = false;
    String errorReason;
    switch (error) {
      case OTA_AUTH_ERROR:    errorReason = "Auth Failed";    break;
      case OTA_BEGIN_ERROR:   errorReason = "Begin Failed";   break;
      case OTA_CONNECT_ERROR: errorReason = "Connect Failed"; break;
      case OTA_RECEIVE_ERROR: errorReason = "Receive Failed"; break;
      case OTA_END_ERROR:     errorReason = "End Failed";     break;
      default:                errorReason = "Unknown";        break;
    }
    addLogMessage("[ArduinoOTA] Error: " + errorReason);
    publishMqtt(mqttArduinoOtaTaskTopic.c_str(), "{\"event\":\"error\",\"reason\":\"" + errorReason + "\"}"); });

  // Start ArduinoOTA
  ArduinoOTA.begin();
  addLogMessage("[ArduinoOTA] Ready on port " + String(arduinoOtaPort) + " hostname: " + mdnsHostname);
  publishMqtt(mqttArduinoOtaTaskTopic.c_str(),
              "{\"event\":\"ready\",\"port\":" + String(arduinoOtaPort) +
              ",\"hostname\":\"" + mdnsHostname + "\"}");

  // Handle OTA requests
  while (true)
  {
    ArduinoOTA.handle();
    vTaskDelay(10 / portTICK_PERIOD_MS); // Small delay, OTA needs quick response
  }
}
