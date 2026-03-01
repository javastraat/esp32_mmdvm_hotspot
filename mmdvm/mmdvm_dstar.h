/*
 * MMDVM D-Star Protocol Module
 * Handles D-Star protocol processing
 */

#ifndef MMDVM_DSTAR_H
#define MMDVM_DSTAR_H

#include <Arduino.h>
#include "../include/config.h"

// Task handle
extern TaskHandle_t dstarTaskHandle;

// Function to create and start D-Star task
void initDstarTask();

// D-Star task function
void dstarTask(void *parameter);

#endif // MMDVM_DSTAR_H
