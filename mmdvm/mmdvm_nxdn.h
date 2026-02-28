/*
 * MMDVM NXDN Protocol Module
 * Handles NXDN protocol processing
 */

#ifndef MMDVM_NXDN_H
#define MMDVM_NXDN_H

#include <Arduino.h>
#include "../include/config.h"

// Task handle
extern TaskHandle_t nxdnTaskHandle;

// Function to create and start NXDN task
void initNxdnTask();

// NXDN task function
void nxdnTask(void *parameter);

#endif // MMDVM_NXDN_H
