/*
 * RGBLedController.h - RGB LED Status Indicator for ESP32 MMDVM Hotspot
 *
 * Controls RGB LEDs to show network and DMR activity status:
 * - Base color shows network status
 * - Brightness changes show TX/RX activity
 */

#ifndef RGB_LED_CONTROLLER_H
#define RGB_LED_CONTROLLER_H

#include <Arduino.h>

// LED Status States
enum class RGBLedStatus {
  DISCONNECTED,      // Red - No network connection
  AP_MODE,           // Purple - Access Point mode
  CONNECTING,        // Blue flashing - Connecting to network
  NETWORK_CONNECTED, // Dim blue - Network connected, idle
  TRANSMITTING,      // Bright green - Transmitting
  RECEIVING,         // Bright red - Receiving
  IDLE_CONNECTED     // Dim blue - Connected and idle
};

class RGBLedController {
private:
  uint8_t redPin;
  uint8_t greenPin;
  uint8_t bluePin;

  uint8_t idleBrightness;
  uint8_t activeBrightness;

  bool enabled;
  RGBLedStatus currentStatus;
  unsigned long lastBlinkTime;
  bool blinkState;

  // PWM settings for ESP32
  const uint16_t PWM_FREQ = 5000;     // 5 kHz PWM frequency
  const uint8_t PWM_RESOLUTION = 8;   // 8-bit resolution (0-255)

  // Apply brightness scaling to color value
  uint8_t applyBrightness(uint8_t colorValue, uint8_t brightness) {
    return (colorValue * brightness) / 255;
  }

  // Set RGB color with PWM (ESP32 Arduino Core 3.x API)
  void setRGB(uint8_t red, uint8_t green, uint8_t blue) {
    ledcWrite(redPin, red);
    ledcWrite(greenPin, green);
    ledcWrite(bluePin, blue);
  }

public:
  RGBLedController(uint8_t rPin, uint8_t gPin, uint8_t bPin,
                   uint8_t idleBright = 10, uint8_t activeBright = 50)
    : redPin(rPin), greenPin(gPin), bluePin(bPin),
      idleBrightness(idleBright), activeBrightness(activeBright),
      enabled(true), currentStatus(RGBLedStatus::DISCONNECTED),
      lastBlinkTime(0), blinkState(false) {
  }

  // Initialize LED pins and PWM
  void begin() {
    if (!enabled) return;

    // Configure PWM for each color channel (ESP32 Arduino Core 3.x API)
    ledcAttach(redPin, PWM_FREQ, PWM_RESOLUTION);
    ledcAttach(greenPin, PWM_FREQ, PWM_RESOLUTION);
    ledcAttach(bluePin, PWM_FREQ, PWM_RESOLUTION);

    // Start with disconnected status (red)
    setStatus(RGBLedStatus::DISCONNECTED);
  }

  // Set LED status
  void setStatus(RGBLedStatus status) {
    if (!enabled) return;

    currentStatus = status;
    updateLED();
  }

  // Update LED based on current status
  void updateLED() {
    if (!enabled) {
      setRGB(0, 0, 0);
      return;
    }

    uint8_t r = 0, g = 0, b = 0;
    uint8_t brightness = idleBrightness;

    switch (currentStatus) {
      case RGBLedStatus::DISCONNECTED:
        // Red - No connection
        r = 255;
        brightness = idleBrightness;
        break;

      case RGBLedStatus::AP_MODE:
        // Purple - Access Point mode
        r = 128;
        b = 128;
        brightness = idleBrightness;
        break;

      case RGBLedStatus::CONNECTING:
        // Blue blinking - Connecting
        b = 255;
        brightness = blinkState ? activeBrightness : 0;
        break;

      case RGBLedStatus::NETWORK_CONNECTED:
      case RGBLedStatus::IDLE_CONNECTED:
        // Dim blue - Connected and idle
        b = 255;
        brightness = idleBrightness;
        break;

      case RGBLedStatus::TRANSMITTING:
        // Bright green - Transmitting
        g = 255;
        brightness = activeBrightness;
        break;

      case RGBLedStatus::RECEIVING:
        // Bright red - Receiving
        r = 255;
        brightness = activeBrightness;
        break;
    }

    // Apply brightness and set LED
    setRGB(applyBrightness(r, brightness),
           applyBrightness(g, brightness),
           applyBrightness(b, brightness));
  }

  // Call this regularly in loop() for blinking effects
  void update() {
    if (!enabled) return;

    // Handle blinking for connecting state
    if (currentStatus == RGBLedStatus::CONNECTING) {
      unsigned long now = millis();
      if (now - lastBlinkTime > 500) {  // Blink every 500ms
        lastBlinkTime = now;
        blinkState = !blinkState;
        updateLED();
      }
    }
  }

  // Enable/disable LED
  void setEnabled(bool enable) {
    enabled = enable;
    if (!enabled) {
      setRGB(0, 0, 0);
    } else {
      updateLED();
    }
  }

  // Set brightness levels
  void setBrightness(uint8_t idleBright, uint8_t activeBright) {
    idleBrightness = idleBright;
    activeBrightness = activeBright;
    updateLED();
  }

  // Turn off LED
  void off() {
    setRGB(0, 0, 0);
  }
};

#endif // RGB_LED_CONTROLLER_H
