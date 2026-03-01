/*
 * DAPNET Network Client Task
 * Connects to a DAPNET server via TCP and queues received pages for POCSAG TX
 */

#ifndef MMDVM_DAPNET_H
#define MMDVM_DAPNET_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include "../include/config.h"

extern TaskHandle_t dapnetTaskHandle;
extern volatile bool dapnetLoggedIn;

void initDapnetTask();
void dapnetTask(void* parameter);
String getDapnetMessageHistoryJson();

#endif // MMDVM_DAPNET_H
