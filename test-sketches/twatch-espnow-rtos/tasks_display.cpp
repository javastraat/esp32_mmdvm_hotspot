#include "tasks_display.h"
#include <Arduino.h>

TaskHandle_t displayTaskHandle = NULL;

void displayTask(void *parameter) {
  // TODO: Add display/UI logic here
  for (;;) {
    // Example: vTaskDelay(100 / portTICK_PERIOD_MS);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void initDisplayTask() {
  xTaskCreatePinnedToCore(displayTask, "DisplayTask", 4096, NULL, 1, &displayTaskHandle, 1);
}
