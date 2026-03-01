/*
 * MMDVM YSF (System Fusion) Protocol Module
 * Handles Yaesu System Fusion protocol processing
 */

#ifndef MMDVM_YSF_H
#define MMDVM_YSF_H

#include <Arduino.h>
#include "../include/config.h"

// Task handle
extern TaskHandle_t ysfTaskHandle;

// Function to create and start YSF task
void initYsfTask();

// YSF task function
void ysfTask(void *parameter);

#endif // MMDVM_YSF_H
