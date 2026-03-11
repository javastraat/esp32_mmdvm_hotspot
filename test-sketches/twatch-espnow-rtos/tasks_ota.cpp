#include "tasks_ota.h"
#include <Arduino.h>

TaskHandle_t otaTaskHandle = NULL;

void otaTask(void *parameter) {
  // TODO: Add OTA update logic here
  for (;;) {
    // Example: vTaskDelay(100 / portTICK_PERIOD_MS);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void initOtaTask() {
  xTaskCreatePinnedToCore(otaTask, "OtaTask", 4096, NULL, 1, &otaTaskHandle, 1);
}
