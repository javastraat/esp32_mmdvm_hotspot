// buttons.cpp — Debounced button handler.
#include "buttons.h"
#include "globals.h"
#include "buzzer.h"
#include "display.h"
#include "mqtt.h"

void triggerButton(int i) {
  resetScreensaverIdle();
  switch (i) {
    case 0:  // Left — reserved
      LOG("[BTN] Left\n");
      buzzerClick();
      break;
    case 1:  // Middle — toggle auto-brightness
      autoBrightnessEnabled = !autoBrightnessEnabled;
      if (!autoBrightnessEnabled)
        FastLED.setBrightness(currentBrightness);
      LOG("[BTN] Middle — auto brightness: %s\n",
        autoBrightnessEnabled ? "ON" : "OFF");
      buzzerClick();
      break;
    case 2:  // Right — cycle display mode (requires SHT31)
      if (sht31Available) {
        displayMode = (DisplayMode)((displayMode + 1) % MODE_COUNT);
        modeActiveUntil = (displayMode != MODE_CLOCK) ? millis() + MODE_TIMEOUT_MS : 0;
        LOG("[BTN] Right — mode: %s\n",
          displayMode == MODE_TEMP ? "temp" :
          displayMode == MODE_HUMIDITY ? "humidity" : "clock");
      } else {
        LOG("[BTN] Right — no SHT31 sensor\n");
      }
      buzzerClick();
      break;
  }
  mqttNotifyState();  // push updated state to HA immediately
}

void loopButtons() {
  static bool          lastState[3]  = {HIGH, HIGH, HIGH};
  static bool          fired[3]      = {false, false, false};
  static unsigned long lastChange[3] = {0, 0, 0};
  static const uint8_t pins[3]       = {BTN_LEFT, BTN_MIDDLE, BTN_RIGHT};

  for (int i = 0; i < 3; i++) {
    bool state = digitalRead(pins[i]);
    if (state != lastState[i]) {
      lastChange[i] = millis();
      lastState[i]  = state;
      fired[i]      = false;
    }
    if (!fired[i] && state == LOW && millis() - lastChange[i] >= BTN_DEBOUNCE_MS) {
      fired[i] = true;
      triggerButton(i);
    }
  }
}
