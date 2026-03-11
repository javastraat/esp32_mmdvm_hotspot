#include "tasks_espnow.h"
#include <Arduino.h>

TaskHandle_t espnowTaskHandle = NULL;

void espnowTask(void *parameter) {
  // TODO: Add ESP-NOW receive/parse logic here
  for (;;) {
    // Example: vTaskDelay(100 / portTICK_PERIOD_MS);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void initEspNowTask() {
  xTaskCreatePinnedToCore(espnowTask, "EspNowTask", 4096, NULL, 1, &espnowTaskHandle, 1);
}
