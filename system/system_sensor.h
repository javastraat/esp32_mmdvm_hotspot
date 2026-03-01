/*
 * Sensor System Module
 * Handles sensor reading and monitoring
 */

#ifndef SYSTEM_SENSOR_H
#define SYSTEM_SENSOR_H

#include <Arduino.h>
#include "../include/config.h"

// Task handle
extern TaskHandle_t sensorTaskHandle;

// Function to create and start sensor task
void initSensorTask();

// Sensor task function
void sensorTask(void *parameter);

#endif // SYSTEM_SENSOR_H
