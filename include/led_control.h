/*
 * led_control.h - LED Status Control for ESP32 MMDVM Hotspot
 *
 * Handles status LED patterns and updates
 *
 * NOTE: Function implementations remain in main .ino file
 * This header provides declarations and extern variables for LED control functions
 */

#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>
#include "config.h"

// LED mode enum
enum class LED_MODE {
  OFF,
  STEADY,      // Connected to WiFi
  FAST_BLINK,  // Connecting to WiFi
  SLOW_BLINK   // Access Point mode
};

// External LED state variables (defined in main .ino)
extern LED_MODE currentLedMode;
extern bool ledState;
extern unsigned long lastLedToggle;

// Function declarations
void setLEDMode(LED_MODE mode);
void updateStatusLED();

#endif // LED_CONTROL_H
