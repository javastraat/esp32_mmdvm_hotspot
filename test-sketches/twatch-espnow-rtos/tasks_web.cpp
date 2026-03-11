#include "tasks_web.h"
#include <Arduino.h>

TaskHandle_t webTaskHandle = NULL;

void webTask(void *parameter) {
  // TODO: Add web server logic here
  for (;;) {
    // Example: vTaskDelay(100 / portTICK_PERIOD_MS);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void initWebTask() {
  xTaskCreatePinnedToCore(webTask, "WebTask", 8192, NULL, 1, &webTaskHandle, 1);
}
