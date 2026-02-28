/*
 * Ethernet System Module
 * Handles Ethernet connection and monitoring via SPI (W5500)
 */

#ifndef SYSTEM_ETH_H
#define SYSTEM_ETH_H

#include <Arduino.h>
#include <SPI.h>
#include "../include/config.h"

#if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(3, 0, 0)
#include <ETHClass2.h> // Use modified ETHClass for older versions
#define ETH ETH2
#else
#include <ETH.h>
#endif

// Task handle
extern TaskHandle_t ethTaskHandle;

// Ethernet status
extern volatile bool ethConnected;

// Function to create and start Ethernet task
void initEthTask();

// Ethernet task function
void ethTask(void *parameter);

// Ethernet event handlers
void onEthEvent(arduino_event_id_t event);

#endif // SYSTEM_ETH_H
