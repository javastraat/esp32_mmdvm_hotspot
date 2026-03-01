/*
 * MMDVM P25 Protocol Module
 * Handles P25 (Project 25) protocol processing
 */

#ifndef MMDVM_P25_H
#define MMDVM_P25_H

#include <Arduino.h>
#include "../include/config.h"

// Task handle
extern TaskHandle_t p25TaskHandle;

// Function to create and start P25 task
void initP25Task();

// P25 task function
void p25Task(void *parameter);

#endif // MMDVM_P25_H
