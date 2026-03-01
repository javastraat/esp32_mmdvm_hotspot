/*
 * MMDVM NXDN Protocol Module Implementation
 */

#include "mmdvm/mmdvm_nxdn.h"
#include "system/system_logger.h"
#include <WiFi.h>
#include "system/system_eth.h"

extern String mqttNxdnTaskTopic;

TaskHandle_t nxdnTaskHandle = NULL;

void initNxdnTask()
{
  BaseType_t result = xTaskCreatePinnedToCore(
      nxdnTask,
      "NXDN Protocol",
      MMDVM_NXDN_STACK,
      NULL,
      MMDVM_NXDN_PRIORITY,
      &nxdnTaskHandle,
      1 // Run on core 1
  );
  if (result != pdPASS)
    log_e("[NXDN] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());
}

/*
 * TASK: NXDN Protocol Handler
 * Priority: High - Real-time digital voice protocol
 *
 * Handles NXDN protocol processing
 * TODO: Add NXDN encoding/decoding, reflector connections
 */
void nxdnTask(void *parameter)
{
  addLogMessage("[NXDN Task] Started - waiting for network connection...");

  // Wait for any network connection (WiFi OR Ethernet)
  while (WiFi.status() != WL_CONNECTED && !ethConnected)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  addLogMessage("[NXDN Task] Network connected - initializing NXDN protocol");

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

  addLogMessage("[NXDN Task] Initialized - Ready for NXDN traffic");

  // Main loop - placeholder for NXDN protocol processing
  while (true)
  {
    // TODO: Process NXDN frames
    // TODO: Handle reflector communication
    // TODO: Network bridge communication

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
