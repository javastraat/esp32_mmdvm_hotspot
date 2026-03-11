#include "tasks_wifi.h"
#include <Arduino.h>

TaskHandle_t wifiTaskHandle = NULL;

void wifiTask(void *parameter) {
  // TODO: Add WiFi/AP management logic here
  for (;;) {
    // Example: vTaskDelay(100 / portTICK_PERIOD_MS);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void initWifiTask() {
  xTaskCreatePinnedToCore(wifiTask, "WifiTask", 4096, NULL, 1, &wifiTaskHandle, 1);
}
