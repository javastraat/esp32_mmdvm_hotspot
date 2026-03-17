#pragma once
#include <Arduino.h>

// setupOTA() — called from receiver.cpp once WiFi connects.
// Initialises ArduinoOTA and starts the web server.
void setupOTA();

// initWebTask() — called from setup() in ulanzi-espnow.ino.
// Starts the FreeRTOS task that drives ArduinoOTA.handle() + webServer.handleClient().
void initWebTask();
