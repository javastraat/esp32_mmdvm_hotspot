/*
 * system_ntp.cpp - NTP Time Synchronization Implementation
 */

// ...existing code...
#include "system/system_ntp.h"
#include "system/system_logger.h"
#include "system/system_eth.h"
#include "include/config.h"
#include "system/service_mqtt.h"
#include <WiFi.h>

extern String mqttNtpTaskTopic;

// NTP status variables
volatile bool ntpSynced = false;
time_t lastSyncTime = 0;

// Task handle for stack monitoring
TaskHandle_t ntpTaskHandle = NULL;

// Forward declaration
void ntpTask(void *parameter);

/*
 * Initialize NTP time synchronization task
 */
void initNtpTask()
{
  // Create NTP task on core 0 with normal priority
  BaseType_t result = xTaskCreatePinnedToCore(
      ntpTask,           // Task function
      "NTP Task",        // Task name
      NTP_TASK_STACK,    // Stack size
      NULL,              // Parameters
      NTP_TASK_PRIORITY, // Priority
      &ntpTaskHandle,    // Task handle
      0                  // Core 0
  );
  if (result != pdPASS)
    log_e("[NTP] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());

  addLogMessage("[NTP Task] Started");
  publishMqtt(mqttNtpTaskTopic.c_str(), "{\"event\":\"started\"}");
}

/*
 * NTP synchronization task
 * Syncs time when network is available and periodically updates
 */
void ntpTask(void *parameter)
{
  // Wait for any network connection (WiFi OR Ethernet)
  while (WiFi.status() != WL_CONNECTED && !ethConnected)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  addLogMessage("[NTP Task] Network connected, initializing time sync");
  publishMqtt(mqttNtpTaskTopic.c_str(), "{\"event\":\"network_ready\"}");

  // Configure NTP with timezone and DST
  configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  // Wait for first time sync — keep retrying every 5s until success (no hard limit)
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo))
  {
    addLogMessage("[NTP Task] Waiting for time sync...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    // Re-call configTime if network changed (handles late DHCP / reconnect)
    configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  }

  ntpSynced = true;
  lastSyncTime = time(NULL);
  addLogMessage("[NTP Task] Time synced: " + getCurrentDateTime());
  publishMqtt(mqttNtpTaskTopic.c_str(), "{\"event\":\"synced\",\"datetime\":\"" + getCurrentDateTime() + "\"}");

  // Periodic re-sync loop (hourly)
  while (true)
  {
    vTaskDelay(NTP_SYNC_INTERVAL_MS / portTICK_PERIOD_MS);

    if (WiFi.status() == WL_CONNECTED || ethConnected)
    {
      struct tm timeinfo;
      if (getLocalTime(&timeinfo))
      {
        ntpSynced = true;
        lastSyncTime = time(NULL);
        addLogMessage("[NTP Task] Time re-synced: " + getCurrentDateTime());
        publishMqtt(mqttNtpTaskTopic.c_str(), "{\"event\":\"resynced\",\"datetime\":\"" + getCurrentDateTime() + "\"}");
      }
      else
      {
        addLogMessage("[NTP Task] Failed to re-sync time");
        publishMqtt(mqttNtpTaskTopic.c_str(), "{\"event\":\"resync_failed\"}");
      }
    }
  }
}

/*
 * Get current time as HH:MM:SS string
 */
String getCurrentTime()
{
  if (!ntpSynced)
    return "--:--:--";

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return "--:--:--";
  }

  char buffer[9];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
  return String(buffer);
}

/*
 * Get current date as YYYY-MM-DD string
 */
String getCurrentDate()
{
  if (!ntpSynced)
    return "-------;--";

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return "------;--";
  }

  char buffer[11];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
  return String(buffer);
}

/*
 * Get current date and time as YYYY-MM-DD HH:MM:SS string
 */
String getCurrentDateTime()
{
  if (!ntpSynced)
    return "---------- --:--:--";

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return "---------- --:--:--";
  }

  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

/*
 * Get Unix timestamp (seconds since epoch)
 */
time_t getUnixTime()
{
  if (!ntpSynced)
    return 0;
  return time(NULL);
}

/*
 * Check if time has been synced
 */
bool isTimeSynced()
{
  return ntpSynced;
}
