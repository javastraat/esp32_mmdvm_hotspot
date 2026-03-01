/*
 * system_mqtt.cpp - MQTT Client Implementation
 */

#include "system/system_mqtt.h"
#include "system/system_logger.h"
#include "system/system_eth.h"
#include "include/config.h"
#include <WiFi.h>
#include <WiFiClient.h>

extern String mqttMqttClientTaskTopic;
extern String mqttHardwareTopic;
extern bool mqttHardwareInfoLog;
extern uint16_t mqttSendHardwareInfo;

// Runtime settings (loaded from NVS in main .ino)
extern String mqttBroker;
extern uint16_t mqttPort;
extern String mqttUser;
extern String mqttPassword;
extern String mqttStatusTopic;
extern String mqttSubscribeTopic;
extern String mqttCommandToken;
extern String mdnsHostname;

// MQTT status variables
bool mqttConnected = false;
unsigned long lastMqttAttempt = 0;

// Task handle for stack monitoring
TaskHandle_t mqttTaskHandle = NULL;

// MQTT client objects
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Forward declarations
void mqttTask(void *parameter);
void reconnectMqtt();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void publishHardwareInfo();

/*
 * Initialize MQTT task
 */
void initMqttTask()
{
  // Configure MQTT client (server is set in reconnectMqtt() so runtime changes take effect)
  mqttClient.setCallback(mqttCallback);

  // Increase buffer size for hardware info JSON (default 256 is too small)
  mqttClient.setBufferSize(1024);

  // Create MQTT task on core 0
  BaseType_t result = xTaskCreatePinnedToCore(
      mqttTask,           // Task function
      "MQTT Task",        // Task name
      MQTT_TASK_STACK,    // Stack size
      NULL,               // Parameters
      MQTT_TASK_PRIORITY, // Priority
      &mqttTaskHandle,    // Task handle
      0                   // Core 0
  );
  if (result != pdPASS)
    log_e("[MQTT] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());

  addLogMessage("[MQTT Task] Started");
}

/*
 * MQTT callback for incoming messages (subscriptions)
 */
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  // String message = "";
  // for (unsigned int i = 0; i < length; i++)
  // {
  //   message += (char)payload[i];
  // }
  String message;
  message.reserve(length);         // ← pre-allocate
  for (unsigned int i = 0; i < length; i++) {
      message += (char)payload[i];
  }

  // Skip announce/info messages published by this device itself
  // (the announce contains "cmd" inside usage/example fields, so don't rely on its absence)
  if (message.indexOf("\"info\":") >= 0)
  {
    return;
  }

  addLogMessage("[MQTT Task] Message received on " + String(topic) + ": " + message);

  // Extract a JSON string field value: finds "key":"value" and returns value
  auto extractJsonString = [](const String &json, const char *key) -> String {
    String search = String("\"") + key + "\"";
    int keyPos = json.indexOf(search);
    if (keyPos < 0) return "";
    int colon = json.indexOf(':', keyPos + search.length());
    if (colon < 0) return "";
    int quote1 = json.indexOf('"', colon + 1);
    if (quote1 < 0) return "";
    int quote2 = json.indexOf('"', quote1 + 1);
    if (quote2 < 0) return "";
    return json.substring(quote1 + 1, quote2);
  };

  // Token is always required — reject if none is configured or if it doesn't match
  if (mqttCommandToken.length() == 0)
  {
    addLogMessage("[MQTT Task] Command rejected: no token configured (set one in MQTT settings)");
    return;
  }
  String token = extractJsonString(message, "token");
  if (token != mqttCommandToken)
  {
    addLogMessage("[MQTT Task] Command rejected: invalid or missing token");
    return;
  }

  // Dispatch command
  String cmd = extractJsonString(message, "cmd");

  if (cmd == "reboot")
  {
    addLogMessage("[MQTT Task] Command: reboot");
    mqttClient.publish(mqttStatusTopic.c_str(), "{\"status\":\"rebooting\"}");
    mqttClient.loop();
    delay(500);
    ESP.restart();
  }
  else if (cmd == "get_hardware")
  {
    addLogMessage("[MQTT Task] Command: get_hardware");
    publishHardwareInfo();
  }
  else if (cmd == "get_status")
  {
    addLogMessage("[MQTT Task] Command: get_status");
    unsigned long uptime = millis() / 1000;
    String status = "{\"status\":\"online\",\"uptime_seconds\":" + String(uptime) + "}";
    mqttClient.publish(mqttStatusTopic.c_str(), status.c_str());
  }
  else
  {
    addLogMessage("[MQTT Task] Command unknown: " + cmd);
  }
}

/*
 * Reconnect to MQTT broker
 */
void reconnectMqtt()
{
  // Only attempt reconnection at configured interval
  if (millis() - lastMqttAttempt < MQTT_RECONNECT_INTERVAL)
  {
    return;
  }

  lastMqttAttempt = millis();

  // Check if any network is available
  if (WiFi.status() != WL_CONNECTED && !ethConnected)
  {
    return;
  }

  addLogMessage("[MQTT Task] Attempting connection to " + mqttBroker + ":" + String(mqttPort));

  // Apply current runtime broker settings (picks up any changes saved via web UI)
  mqttClient.setServer(mqttBroker.c_str(), mqttPort);

  // Use mDNS hostname + chip MAC suffix as client ID to ensure uniqueness per device
  uint32_t chipId = (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF);
  char macSuffix[9];
  snprintf(macSuffix, sizeof(macSuffix), "%08X", chipId);
  String clientId = mdnsHostname + "-" + String(macSuffix);

  // Attempt to connect
  bool connected = false;
  if (mqttUser.length() > 0 && mqttPassword.length() > 0)
  {
    // Connect with authentication
    connected = mqttClient.connect(clientId.c_str(), mqttUser.c_str(), mqttPassword.c_str());
  }
  else
  {
    // Connect without authentication
    connected = mqttClient.connect(clientId.c_str());
  }

  if (connected)
  {
    mqttConnected = true;
    addLogMessage("[MQTT Task] Connected to broker as " + clientId);

    // Subscribe to topic if configured
    if (mqttSubscribeTopic.length() > 0)
    {
      mqttClient.subscribe(mqttSubscribeTopic.c_str());
      addLogMessage("[MQTT Task] Subscribed to: " + mqttSubscribeTopic);

#if MQTT_CMD_ANNOUNCE
      // Announce available commands — token is always required
      String announce = "{\"info\":\"Commands available\","
                        "\"commands\":[\"reboot\",\"get_hardware\",\"get_status\"],"
                        "\"token_required\":true,"
                        "\"usage\":{\"cmd\":\"<command>\",\"token\":\"<your_token>\"},"
                        "\"example\":{\"cmd\":\"reboot\",\"token\":\"<your_token>\"},"
                        "\"client\":\"" + clientId + "\"}";
      mqttClient.publish(mqttSubscribeTopic.c_str(), announce.c_str());
      addLogMessage("[MQTT Task] Command announce published to: " + mqttSubscribeTopic);
#endif
    }

    // Publish connection message
    String connectMsg = "{\"status\":\"connected\",\"client\":\"" + clientId + "\"}";
    mqttClient.publish(mqttStatusTopic.c_str(), connectMsg.c_str());

    // Publish hardware information
    publishHardwareInfo();
  }
  else
  {
    mqttConnected = false;
    addLogMessage("[MQTT Task] Connection failed, rc=" + String(mqttClient.state()));
  }
}

/*
 * MQTT task - maintains connection and handles communication
 */
void mqttTask(void *parameter)
{
  // Wait for any network connection (WiFi OR Ethernet)
  while (WiFi.status() != WL_CONNECTED && !ethConnected)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  addLogMessage("[MQTT Task] Network connected, starting MQTT client");

  // Timing for periodic hardware info publishing
  unsigned long lastHardwareInfoSent = 0;

  // Main MQTT loop
  while (true)
  {
    // Maintain connection
    if (!mqttClient.connected())
    {
      mqttConnected = false;
      reconnectMqtt();
      // Reset timer after reconnection so hardware info is sent immediately next cycle
      lastHardwareInfoSent = 0;
    }
    else
    {
      mqttConnected = true;
      mqttClient.loop();

      // Periodically send hardware info — read global so web UI changes take effect immediately
      unsigned long now = millis();
      if (now - lastHardwareInfoSent >= (unsigned long)mqttSendHardwareInfo * 1000UL)
      {
        publishHardwareInfo();
        lastHardwareInfoSent = now;
      }
    }

    // Check every 100ms
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

/*
 * Publish message to MQTT topic
 */
bool publishMqtt(const char *topic, const char *payload)
{
  if (!mqttConnected || !mqttClient.connected())
  {
    return false;
  }

  bool result = mqttClient.publish(topic, payload);

  if (mqttHardwareInfoLog)
  {
    if (result)
    {
      addLogMessage("[MQTT Task] Published to " + String(topic) + ": " + String(payload));
    }
    else
    {
      addLogMessage("[MQTT Task] Failed to publish to " + String(topic));
    }
  }

  return result;
}

/*
 * Publish message to MQTT topic (String overload)
 */
bool publishMqtt(const char *topic, String payload)
{
  return publishMqtt(topic, payload.c_str());
}

/*
 * Check if MQTT is connected
 */
bool isMqttConnected()
{
  return mqttConnected && mqttClient.connected();
}

/*
 * Publish hardware information to MQTT (called on connect)
 */
void publishHardwareInfo()
{
  if (!mqttConnected || !mqttClient.connected())
  {
    return;
  }

  // Build JSON with hardware information
  String json = "{";

  // System Hardware
  json += "\"chip_model\":\"" + String(ESP.getChipModel()) + "\",";
  json += "\"chip_revision\":" + String(ESP.getChipRevision()) + ",";

  // Chip ID from EFuse MAC
  uint64_t chipId = ESP.getEfuseMac();
  char chipIdStr[18];
  snprintf(chipIdStr, sizeof(chipIdStr), "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
  json += "\"chip_id\":\"" + String(chipIdStr) + "\",";

  json += "\"cpu_cores\":" + String(ESP.getChipCores()) + ",";
  json += "\"cpu_freq_mhz\":" + String(ESP.getCpuFreqMHz()) + ",";

  // CPU Temperature
  float tempC = temperatureRead();
  if (tempC > -40 && tempC < 125)
  {
    json += "\"cpu_temp_c\":" + String(tempC, 1) + ",";
  }

  // Memory
  json += "\"heap_size_kb\":" + String(ESP.getHeapSize() / 1024.0, 1) + ",";
  json += "\"free_heap_kb\":" + String(ESP.getFreeHeap() / 1024.0, 1) + ",";
  json += "\"min_free_heap_kb\":" + String(ESP.getMinFreeHeap() / 1024.0, 1) + ",";
  json += "\"max_alloc_heap_kb\":" + String(ESP.getMaxAllocHeap() / 1024.0, 1) + ",";

  // PSRAM (if available)
  if (ESP.getPsramSize() > 0)
  {
    json += "\"psram_size_mb\":" + String(ESP.getPsramSize() / 1024 / 1024) + ",";
    json += "\"free_psram_kb\":" + String(ESP.getFreePsram() / 1024.0, 1) + ",";
  }

  // Flash Storage
  json += "\"flash_size_mb\":" + String(ESP.getFlashChipSize() / 1024 / 1024) + ",";
  json += "\"flash_speed_mhz\":" + String(ESP.getFlashChipSpeed() / 1000000) + ",";

  // Flash Mode
  const char *flashModes[] = {"QIO", "QOUT", "DIO", "DOUT", "Fast Read", "Slow Read"};
  FlashMode_t flashMode = ESP.getFlashChipMode();
  String flashModeStr = (flashMode < 6) ? flashModes[flashMode] : "Unknown";
  json += "\"flash_mode\":\"" + flashModeStr + "\",";

  json += "\"sketch_size_kb\":" + String(ESP.getSketchSize() / 1024.0, 1) + ",";
  json += "\"free_sketch_space_kb\":" + String(ESP.getFreeSketchSpace() / 1024.0, 1) + ",";

  // Software
  unsigned long uptimeSeconds = millis() / 1000;
  json += "\"uptime_seconds\":" + String(uptimeSeconds) + ",";

  // Reset reason
  esp_reset_reason_t resetReason = esp_reset_reason();
  const char *resetReasons[] = {"Unknown", "Power-on", "External", "Software", "Panic", "Interrupt WDT", "Task WDT", "Other WDT", "Deepsleep", "Brownout", "SDIO"};
  String resetReasonStr = (resetReason < 11) ? resetReasons[resetReason] : "Unknown";
  json += "\"reset_reason\":\"" + resetReasonStr + "\",";

  json += "\"sdk_version\":\"" + String(ESP.getSdkVersion()) + "\",";

#ifdef ESP_ARDUINO_VERSION_STR
  json += "\"arduino_core\":\"" + String(ESP_ARDUINO_VERSION_STR) + "\",";
#endif

  json += "\"build_date\":\"" + String(__DATE__) + " " + String(__TIME__) + "\",";

  // Network info
  json += "\"wifi_ssid\":\"" + WiFi.SSID() + "\",";
  json += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"ip_address\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"mac_address\":\"" + WiFi.macAddress() + "\"";

  json += "}";

  // Publish to hardware topic
  if (mqttClient.publish(mqttHardwareTopic.c_str(), json.c_str()))
  {
    if (mqttHardwareInfoLog)
      addLogMessage(String("[MQTT Task] Hardware info published to ") + mqttHardwareTopic);
  }
  else
  {
    addLogMessage("[MQTT Task] Failed to publish hardware info");
  }
}
