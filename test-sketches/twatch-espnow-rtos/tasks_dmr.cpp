#include "tasks_dmr.h"
#include <Arduino.h>

TaskHandle_t dmrTaskHandle = NULL;

void dmrTask(void *parameter) {
  // TODO: Add DMR message handling logic here
  for (;;) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void initDmrTask() {
  xTaskCreatePinnedToCore(dmrTask, "DmrTask", 4096, NULL, 1, &dmrTaskHandle, 1);
}
