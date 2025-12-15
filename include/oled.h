/*
 * oled.h - OLED Display Control for ESP32 MMDVM Hotspot
 *
 * Handles OLED display initialization, updates, and power management
 *
 * NOTE: Function implementations remain in main .ino file
 * This header provides declarations and extern variables for OLED functions
 */

#ifndef OLED_H
#define OLED_H

#include <Arduino.h>
#include "config.h"

#if ENABLE_OLED
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// External display object (defined in main .ino)
extern Adafruit_SSD1306 display;

// External OLED state variables (defined in main .ino)
extern unsigned long lastOLEDUpdate;
extern unsigned long lastNetworkToggle;
extern bool oledShowEthernet;
extern int oledActiveSlot;
extern int oledHeaderCycle;
extern bool oledDisplayOn;
extern bool oledAutoBlankEnabled;
extern unsigned long oledBlankTimeout;
extern unsigned long lastOLEDActivity;
extern bool oledBlankingActive;

// External DMR activity structures (defined in web/pages/home.h)
struct DMRActivity;  // Forward declaration
extern DMRActivity dmrActivity[2];

// External system state variables
extern bool wifiConnected;
extern bool apMode;
extern bool mmdvmReady;
extern bool dmrLoggedIn;
extern String dmrLoginStatus;
extern String dmr_callsign;
extern uint32_t currentTalkgroup;

#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
extern bool eth_connected;
#endif

// External mutex for display access
extern SemaphoreHandle_t displayMutex;

// External logging function
extern void logSerial(String message);

// Function declarations
void setupOLED();
void updateBootStatus(String status);
void updateOLEDStatus();
void displayBootLogo();
void displayESP32Logo();
void testdrawbitmap();
void setOLEDPower(bool on);
void toggleOLEDPower();

#endif // ENABLE_OLED

#endif // OLED_H
