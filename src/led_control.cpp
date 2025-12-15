/*
 * led_control.cpp - LED Status Control
 *
 * Implementation of LED control functions for status indication
 */

#include "../include/led_control.h"

void setLEDMode(LED_MODE mode) {
  currentLedMode = mode;
  lastLedToggle = millis();

  // Set initial state based on mode
  if (mode == LED_MODE::STEADY) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    ledState = true;
  } else if (mode == LED_MODE::OFF) {
    digitalWrite(STATUS_LED_PIN, LOW);
    ledState = false;
  }
}

void updateStatusLED() {
  unsigned long currentMillis = millis();
  unsigned long interval;

  switch (currentLedMode) {
    case LED_MODE::OFF:
      // LED stays off
      break;

    case LED_MODE::STEADY:
      // LED stays on
      break;

    case LED_MODE::FAST_BLINK:
      // Fast blink (200ms interval) - connecting to WiFi
      interval = 200;
      if (currentMillis - lastLedToggle >= interval) {
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState ? HIGH : LOW);
        lastLedToggle = currentMillis;
      }
      break;

    case LED_MODE::SLOW_BLINK:
      // Slow blink (1000ms interval) - Access Point mode
      interval = 1000;
      if (currentMillis - lastLedToggle >= interval) {
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState ? HIGH : LOW);
        lastLedToggle = currentMillis;
      }
      break;
  }
}
