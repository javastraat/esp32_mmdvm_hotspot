#include "tasks_dapnet.h"
#include <Arduino.h>

TaskHandle_t dapnetTaskHandle = NULL;

void dapnetTask(void *parameter) {
  // TODO: Add DAPNET message handling logic here
  for (;;) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void initDapnetTask() {
  xTaskCreatePinnedToCore(dapnetTask, "DapnetTask", 4096, NULL, 1, &dapnetTaskHandle, 1);
}
