/*
 * oled.h - OLED Display Control for ESP32 MMDVM Hotspot
 *
 * Handles OLED display initialization, updates, and power management
 *
 * NOTE: Function implementations are in oled.cpp
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
#include <WiFi.h>
#include "oled_graphics.h"

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

// DMR Activity structure (matches definition in web/pages/home.h)
#ifndef DMR_ACTIVITY_STRUCT_DEFINED
#define DMR_ACTIVITY_STRUCT_DEFINED
struct DMRActivity {
  uint32_t srcId;
  uint32_t dstId;
  uint8_t slotNo;
  bool isGroup;
  String frameType;
  String srcCallsign;
  String srcName;
  String srcCity;
  String srcCountry;
  unsigned long lastUpdate;
  unsigned long startTime;
  bool active;
};
#endif

// External DMR activity arrays (defined in main .ino)
extern DMRActivity dmrActivity[2];

// External system state variables
extern bool wifiConnected;
extern bool apMode;
extern bool mmdvmReady;
extern bool dmrLoggedIn;
extern String dmrLoginStatus;
extern String dmr_callsign;
extern uint32_t currentTalkgroup;
extern bool enable_oled;
extern bool mode_dmr_enabled;
extern bool mode_dstar_enabled;
extern bool mode_ysf_enabled;
extern bool mode_p25_enabled;
extern bool mode_nxdn_enabled;
extern bool mode_pocsag_enabled;

#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
extern bool eth_connected;
#include <ETH.h>
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
void displayBitmap();
void setOLEDPower(bool on);
void toggleOLEDPower();

#else
// Stub function declarations when OLED is disabled at compile time
void setupOLED();
void updateBootStatus(String status);
void updateOLEDStatus();
void setOLEDPower(bool on);
void toggleOLEDPower();
#endif // ENABLE_OLED

#endif // OLED_H
