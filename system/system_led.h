/*
 * LED System Module
 * Handles LED blinking for visual feedback
 */

#ifndef SYSTEM_LED_H
#define SYSTEM_LED_H

#include <Arduino.h>
#include "../include/config.h"

// Task handle
extern TaskHandle_t ledTaskHandle;

// Function to create and start LED task
void initLedTask();

// LED task function
void ledTask(void *parameter);

#endif // SYSTEM_LED_H
