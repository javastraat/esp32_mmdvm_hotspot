/*
 * MMDVM D-Star Protocol Module Implementation
 */

#include "mmdvm/mmdvm_dstar.h"
#include "system/system_logger.h"
#include <WiFi.h>
#include "system/system_eth.h"

extern String mqttDstarTaskTopic;

TaskHandle_t dstarTaskHandle = NULL;

void initDstarTask()
{
  BaseType_t result = xTaskCreatePinnedToCore(
      dstarTask,
      "D-Star Protocol",
      MMDVM_DSTAR_STACK,
      NULL,
      MMDVM_DSTAR_PRIORITY,
      &dstarTaskHandle,
      1 // Run on core 1
  );
  if (result != pdPASS)
    log_e("[D-Star] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());
}

/*
 * TASK: D-Star Protocol Handler
 * Priority: High - Real-time digital voice protocol
 *
 * Handles D-Star protocol processing
 * TODO: Add D-Star encoding/decoding, slow data, reflector connections
 */
void dstarTask(void *parameter)
{
  addLogMessage("[D-Star Task] Started - waiting for network connection...");

  // Wait for any network connection (WiFi OR Ethernet)
  while (WiFi.status() != WL_CONNECTED && !ethConnected)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  addLogMessage("[D-Star Task] Network connected - initializing D-Star protocol");

  // Wait 5 seconds before blinking to separate from LED task
  vTaskDelay(5000 / portTICK_PERIOD_MS);

  // Blink LED 3 times to show task is running
  pinMode(LED_PIN, OUTPUT);
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    digitalWrite(LED_PIN, LOW);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  addLogMessage("[D-Star Task] Initialized - Ready for D-Star traffic");

  // Main loop - placeholder for D-Star protocol processing
  while (true)
  {
    // TODO: Process D-Star frames
    // TODO: Handle slow data processing
    // TODO: Reflector/gateway communication

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
