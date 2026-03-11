// Main RTOS entry point for TWatch ESP-NOW RTOS version

#include "tasks_display.h"
#include "tasks_espnow.h"
#include "tasks_wifi.h"
#include "tasks_web.h"
#include "tasks_ota.h"
#include "tasks_dapnet.h"
#include "tasks_dmr.h"

void setup() {
  // Initialize all modules
  initDisplayTask();
  initEspNowTask();
  initWifiTask();
  initWebTask();
  initOtaTask();
  initDapnetTask();
  initDmrTask();
}

void loop() {
  // Nothing here, all logic is in RTOS tasks
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
