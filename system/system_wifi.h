/*
 * WiFi System Module
 * Handles WiFi connection and monitoring
 */

#ifndef SYSTEM_WIFI_H
#define SYSTEM_WIFI_H

#include <WiFi.h>
#include <DNSServer.h>
#include "../include/config.h"

// Task handle
extern TaskHandle_t wifiTaskHandle;

// WiFi AP state
extern bool softAPActive;

// Captive portal DNS server (active only while softAPActive)
extern DNSServer dnsServer;

// Function to create and start WiFi task
void initWiFiTask();

// WiFi task function
void wifiTask(void *parameter);

#endif // SYSTEM_WIFI_H
