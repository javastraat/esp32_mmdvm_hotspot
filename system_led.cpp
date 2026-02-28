/*
 * LED System Module Implementation
 */

#include "system/system_led.h"
#include "system/system_logger.h"
#include "system/system_mqtt.h"
#include <WiFi.h>
#include "system/system_eth.h"
#include "include/sdcard_handlers.h"

extern String mqttLedTaskTopic; // MQTT topic for LED task status updates

TaskHandle_t ledTaskHandle = NULL;

void initLedTask()
{
  BaseType_t result = xTaskCreatePinnedToCore(
      ledTask,
      "LED Task",
      LED_TASK_STACK,
      NULL,
      LED_TASK_PRIORITY,
      &ledTaskHandle,
      0 // Run on core 0 (less critical tasks)
  );
  if (result != pdPASS)
    log_e("[LED] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());
}

/*
 * TASK: LED Status Indicator
 * Priority: Low - Visual indicator, not critical
 *
 * LED behavior based on network status and download activity:
 * - Fast blinking (100ms): Database download in progress
 * - Solid ON: Network connected
 * - Normal blinking: Network disconnected or connecting
 */
void ledTask(void *parameter)
{
  addLogMessage("[LED Task] Started");
  publishMqtt(mqttLedTaskTopic.c_str(), "{\"event\":\"started\"}");
  pinMode(LED_PIN, OUTPUT);

  while (true)
  {
    // Check if downloading database
    bool isDownloading = csvDownloadActive || sqliteDownloadActive;

    // Check if ANY network is connected
    bool isConnected = (WiFi.status() == WL_CONNECTED) || ethConnected;

    if (isDownloading)
    {
      // Fast blink during download (100ms on, 100ms off)
      digitalWrite(LED_PIN, HIGH);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      digitalWrite(LED_PIN, LOW);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    else if (isConnected)
    {
      // Network connected - LED stays on
      digitalWrite(LED_PIN, HIGH);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    else
    {
      // Network disconnected or connecting - LED blinks
      digitalWrite(LED_PIN, HIGH);
      vTaskDelay(LED_ON_TIME / portTICK_PERIOD_MS);
      digitalWrite(LED_PIN, LOW);
      vTaskDelay(LED_OFF_TIME / portTICK_PERIOD_MS);
    }
  }
}
