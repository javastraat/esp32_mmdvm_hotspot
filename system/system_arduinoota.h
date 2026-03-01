/*
 * ArduinoOTA System Module
 * Allows firmware upload from Arduino IDE over network
 */

#ifndef SYSTEM_ARDUINOOTA_H
#define SYSTEM_ARDUINOOTA_H

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "../include/config.h"

// Task handle
extern TaskHandle_t arduinoOtaTaskHandle;

// Progress tracking for OLED display
extern volatile bool arduinoOtaUpdateActive;
extern volatile int arduinoOtaProgress;
extern volatile unsigned long arduinoOtaBytesWritten;
extern volatile unsigned long arduinoOtaBytesTotal;

// Function to create and start ArduinoOTA task
void initArduinoOtaTask();

// ArduinoOTA task function
void arduinoOtaTask(void *parameter);

#endif // SYSTEM_ARDUINOOTA_H
