/*
 * GPIO 12/13 Focused MMDVM Test
 *
 * Since GPIO 12/13 made the MMDVM LEDs flash, let's test these pins
 * more thoroughly with every possible baud rate and extended retries
 */

#include <Arduino.h>

#define MMDVM_FRAME_START 0xE0
#define CMD_GET_VERSION   0x00

// All possible baud rates to test
uint32_t baudRates[] = {
  9600, 14400, 19200, 28800, 38400, 57600,
  76800, 115200, 230400, 250000, 460800, 500000, 921600
};
const int numBauds = sizeof(baudRates) / sizeof(uint32_t);

HardwareSerial MMDVMSerial(2);

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n\n");
  Serial.println("==========================================");
  Serial.println("  GPIO 12/13 FOCUSED MMDVM TEST");
  Serial.println("==========================================");
  Serial.println("Testing GPIO 12 (RX) + GPIO 13 (TX)");
  Serial.println("at " + String(numBauds) + " different baud rates");
  Serial.println("==========================================\n");

  delay(2000);

  // Test GPIO 12/13 at every baud rate
  Serial.println("Configuration 1: RX=12, TX=13");
  Serial.println("--------------------------------");
  testPinPair(12, 13);

  Serial.println("\n\nConfiguration 2: RX=13, TX=12 (swapped)");
  Serial.println("----------------------------------------");
  testPinPair(13, 12);

  Serial.println("\n==========================================");
  Serial.println("           TEST COMPLETE");
  Serial.println("==========================================\n");
}

void loop() {
  delay(1000);
}

void testPinPair(int rxPin, int txPin) {
  for (int i = 0; i < numBauds; i++) {
    uint32_t baud = baudRates[i];

    Serial.print("Baud " + String(baud));
    // Pad for alignment
    for (int j = String(baud).length(); j < 7; j++) Serial.print(" ");
    Serial.print(": ");

    // Configure serial
    MMDVMSerial.end();
    delay(100);
    MMDVMSerial.begin(baud, SERIAL_8N1, rxPin, txPin);
    delay(200);

    // Flush old data
    while (MMDVMSerial.available()) {
      MMDVMSerial.read();
    }

    // Try sending command 3 times
    bool success = false;
    int rxCount = 0;  // Move outside loop
    uint8_t rxBuffer[100];

    for (int attempt = 0; attempt < 3; attempt++) {
      // Send GET_VERSION
      uint8_t cmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};
      MMDVMSerial.write(cmd, 3);
      MMDVMSerial.flush();

      // Wait and check for response
      delay(200);

      rxCount = 0;  // Reset for this attempt

      // Poll for response
      for (int poll = 0; poll < 20; poll++) {
        delay(10);
        while (MMDVMSerial.available() && rxCount < 100) {
          rxBuffer[rxCount++] = MMDVMSerial.read();
        }
        if (rxCount > 0) break;
      }

      if (rxCount > 0) {
        // Got data!
        Serial.print("RX(");
        Serial.print(rxCount);
        Serial.print("): ");

        // Show first 20 bytes
        for (int j = 0; j < min(rxCount, 20); j++) {
          if (rxBuffer[j] < 0x10) Serial.print("0");
          Serial.print(rxBuffer[j], HEX);
          Serial.print(" ");
        }

        // Check if valid MMDVM frame
        if (rxBuffer[0] == MMDVM_FRAME_START) {
          Serial.println(" ✓✓✓ VALID FRAME!");
          Serial.println("\n*** SUCCESS! ***");
          Serial.println("RX Pin:    GPIO " + String(rxPin));
          Serial.println("TX Pin:    GPIO " + String(txPin));
          Serial.println("Baud Rate: " + String(baud));
          Serial.println("\nVersion string:");
          Serial.print("  ");
          for (int j = 4; j < min(rxCount, 30); j++) {
            if (rxBuffer[j] >= 32 && rxBuffer[j] < 127) {
              Serial.print((char)rxBuffer[j]);
            }
          }
          Serial.println("\n");
          success = true;
          break;
        } else {
          Serial.println(" (wrong start byte: 0x" + String(rxBuffer[0], HEX) + ")");
        }
      }

      // Small delay before retry
      delay(100);
    }

    if (!success && rxCount == 0) {
      Serial.println("no response");
    }

    delay(200);  // Delay before next baud rate
  }
}
