/*
 * MMDVM P25 Protocol Module Implementation
 */

#include "mmdvm/mmdvm_p25.h"
#include "system/system_logger.h"
#include <WiFi.h>
#include "system/system_eth.h"

extern String mqttP25TaskTopic;

TaskHandle_t p25TaskHandle = NULL;

void initP25Task()
{
  BaseType_t result = xTaskCreatePinnedToCore(
      p25Task,
      "P25 Protocol",
      MMDVM_P25_STACK,
      NULL,
      MMDVM_P25_PRIORITY,
      &p25TaskHandle,
      1 // Run on core 1
  );
  if (result != pdPASS)
    log_e("[P25] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());
}

/*
 * TASK: P25 Protocol Handler
 * Priority: High - Real-time digital voice protocol
 *
 * Handles P25 (Project 25) protocol processing
 * TODO: Add P25 Phase 1/2 support, talkgroup management, trunking
 */
void p25Task(void *parameter)
{
  addLogMessage("[P25 Task] Started - waiting for network connection...");

  // Wait for any network connection (WiFi OR Ethernet)
  while (WiFi.status() != WL_CONNECTED && !ethConnected)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  addLogMessage("[P25 Task] Network connected - initializing P25 protocol");

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

  addLogMessage("[P25 Task] Initialized - Ready for P25 traffic");

  // Main loop - placeholder for P25 protocol processing
  while (true)
  {
    // TODO: Process P25 frames
    // TODO: Handle talkgroup routing
    // TODO: Network bridge communication

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
