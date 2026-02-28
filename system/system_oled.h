// Needed for String type
#include <Arduino.h>
// Set DMR TX user info for OLED display
void setDmrTxUserInfo(const String& callsign, const String& id, const String& name, const String& city = "", const String& state = "", const String& country = "");
void clearDmrTxUserInfo();
// Returns JSON with current DMR call state + last 15 call history
String getDmrActivityJson();
// Set POCSAG TX info for OLED display (shows RIC + message for ~3s) + records TX history
void setPocsagTxInfo(const String& ric, const String& message);
// Returns JSON with last 15 transmitted POCSAG messages
String getPocsagTxHistoryJson();
/*
 * OLED Display System Module
 * Handles OLED display updates and information display
 */

#ifndef SYSTEM_OLED_H
#define SYSTEM_OLED_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "../include/config.h"

// Task handle
extern TaskHandle_t oledTaskHandle;

// OLED display instance
extern Adafruit_SSD1306 display;

// Function to create and start OLED task
void initOledTask();

// OLED task function
void oledTask(void *parameter);

// Display helper functions
void displayWiFiStatus();
void displaySystemInfo();
void displayDownloadProgress();
void displayFirmwareUpdateProgress();
void displayRebootMessage();
void displayFirmwareReadyMessage();
void displayArduinoOtaProgress();

// Firmware update progress tracking (defined in modem_flasher.cpp and system_firmware.cpp)
extern volatile bool modemFirmwareUpdateActive;
extern volatile int modemFirmwareUpdateProgress;
extern volatile unsigned long modemFirmwareBytesTotal;
extern volatile unsigned long modemFirmwareBytesWritten;

extern volatile bool espFirmwareUpdateActive;
extern volatile int espFirmwareUpdateProgress;
extern volatile unsigned long espFirmwareBytesTotal;
extern volatile unsigned long espFirmwareBytesWritten;
extern volatile bool espFirmwareReadyToFlash;

#endif // SYSTEM_OLED_H
