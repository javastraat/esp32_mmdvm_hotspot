/*
 * Web Server System Module
 * Handles HTTP server and request handling
 */

#ifndef SYSTEM_WEBSERVER_H
#define SYSTEM_WEBSERVER_H

#include <WebServer.h>
#include <WiFi.h>
#include "../include/config.h"

// Task handle
extern TaskHandle_t webServerTaskHandle;

// Web server instance (port is fixed at construction time — see webServerTask)
extern WebServer server;

// Shared statistics
extern int requestCount;
extern volatile float cpuUsage;

// Function to create and start web server task
void initWebServerTask();

// Web server task function
void webServerTask(void *parameter);

// HTTP handlers
void handleRoot();
void handleNotFound();
void handleModeSelect();
void handleModeDmr();
void handleModeDstar();
void handleModeYsf();
void handleModeP25();
void handleModeNxdn();
void handleModePocsag();
void handleSystemHotspot();
void handleServiceDapnet();
void handleServiceHampager();
void handleSystemSerial();
void handleSystemStatus();
void handleSystemInfo();
void handleSystemHardware();
void handleSystemNetwork();
void handleSystemModem();
void handleSystemWifi();
void handleServiceMqtt();
void handleServiceTelegram();
void handleServiceEspnow();
void handleSystemSdcard();
void handleSystemFirmware();
void handleSystemAdmin();
void handleSystemBackup();
void handleSystemFiles();


// API handlers
void handleApiLogs();
void handleApiLogsClear();
void handleApiPocsagSend(WebServer &server);

#endif // SYSTEM_WEBSERVER_H
