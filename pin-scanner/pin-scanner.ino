/*
 * FOCUSED MMDVM RX PIN SCANNER
 *
 * Tests specific TX pins (12, 13, 43, 44) with ALL possible RX pins
 * to find the correct RX GPIO for MMDVM UART
 * Monitors GPIO 12/13 for LED activity to confirm MMDVM response
 */

#include <Arduino.h>

// MMDVM Protocol
#define MMDVM_FRAME_START 0xE0
#define CMD_GET_VERSION   0x00

// LED monitoring pins (we know these control MMDVM LEDs)
#define LED_MONITOR_1 12
#define LED_MONITOR_2 13

// Pin combinations to test
struct PinPair {
  int rx;
  int tx;
};

// Baud rates to test (most likely first)
uint32_t baudRates[] = {460800, 115200, 230400, 57600, 38400, 19200, 9600};
const int numBauds = sizeof(baudRates) / sizeof(uint32_t);

// Known TX candidates (from your testing)
int txPins[] = { 13, 43, 44};
const int numTxPins = sizeof(txPins) / sizeof(int);

// All possible RX pins to test
// Exclude: 0 (boot), 6-11 (flash), 18-19 (flash conflict)
int rxPins[] = {
  1, 2, 3, 4, 5,           // Low GPIOs
  12, 13, 14, 15, 16, 17,  // Mid GPIOs
  18,// 19,                  // USB GPIOs
  33, 34, 35, 36, 37, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48  // High GPIOs
};
const int numRxPins = sizeof(rxPins) / sizeof(int);

// We'll generate pairs dynamically
int totalCombos;

HardwareSerial TestSerial(2);
bool foundWorking = false;
int workingRX = -1;
int workingTX = -1;
int workingBaud = -1;

// LED activity counters
volatile unsigned long led1Changes = 0;
volatile unsigned long led2Changes = 0;

void IRAM_ATTR led1ISR() { led1Changes++; }
void IRAM_ATTR led2ISR() { led2Changes++; }

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Setup LED monitors
  pinMode(LED_MONITOR_1, INPUT);
  pinMode(LED_MONITOR_2, INPUT);
  attachInterrupt(digitalPinToInterrupt(LED_MONITOR_1), led1ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(LED_MONITOR_2), led2ISR, CHANGE);

  // Calculate total combinations: 4 TX pins × numRxPins × numBauds
  totalCombos = numTxPins * numRxPins * numBauds;

  Serial.println("\n\n");
  Serial.println("══════════════════════════════════════════════════════");
  Serial.println("     FOCUSED MMDVM RX PIN SCANNER");
  Serial.println("══════════════════════════════════════════════════════");
  Serial.println("Purpose: Find RX pin by testing known TX candidates");
  Serial.println();
  Serial.println("Testing TX pins: 12, 13, 43, 44");
  Serial.println("Against ALL possible RX pins");
  Serial.println("Monitoring GPIO 12/13 for LED activity");
  Serial.println();
  Serial.println("Testing:");
  Serial.println("  " + String(numTxPins) + " TX pins (12,13,43,44)");
  Serial.println("  " + String(numRxPins) + " RX pins to test");
  Serial.println("  " + String(numBauds) + " baud rates each");
  Serial.println("  Total: " + String(totalCombos) + " tests");
  Serial.println("══════════════════════════════════════════════════════\n");
  Serial.println("This will take ~" + String((totalCombos * 350) / 1000 / 60) + " minutes");
  Serial.println("Press RESET to stop at any time\n");

  delay(3000);  // Let MMDVM boot

  // Test each TX pin with all RX pins
  int testCount = 0;
  
  for (int txIdx = 0; txIdx < numTxPins; txIdx++) {
    int txPin = txPins[txIdx];
    
    Serial.println("\n╔═══════════════════════════════════════════╗");
    Serial.println("║  Testing TX=" + String(txPin) + " with all RX pins  ║");
    Serial.println("╚═══════════════════════════════════════════╝");
    
    for (int rxIdx = 0; rxIdx < numRxPins; rxIdx++) {
      int rxPin = rxPins[rxIdx];
      
      // Skip if same pin (e.g., TX=12, RX=12)
      if (rxPin == txPin) continue;
      
      // Show progress every 20 tests
      if (testCount % 20 == 0 && testCount > 0) {
        Serial.println("\n[Progress: " + String(testCount) + "/" + String(totalCombos) + 
                      " tests | " + String(testCount * 100 / totalCombos) + "%]");
        Serial.println("LED Activity: GPIO12=" + String(led1Changes) + " GPIO13=" + String(led2Changes));
      }
      
      // Test this pair at all baud rates
      for (int baudIdx = 0; baudIdx < numBauds; baudIdx++) {
        testCount++;
        
        PinPair pins = {rxPin, txPin};
        bool success = testConfiguration(pins, baudRates[baudIdx], testCount);
        
        if (success) {
          foundWorking = true;
          workingRX = rxPin;
          workingTX = txPin;
          workingBaud = baudRates[baudIdx];

          Serial.println("\n╔════════════════════════════════════════════════╗");
          Serial.println("║     ✓✓✓ SUCCESS! WORKING CONFIG FOUND ✓✓✓     ║");
          Serial.println("╚════════════════════════════════════════════════╝");
          Serial.println("RX Pin:    GPIO " + String(workingRX));
          Serial.println("TX Pin:    GPIO " + String(workingTX));
          Serial.println("Baud Rate: " + String(workingBaud));
          Serial.println("LED Activity: GPIO12=" + String(led1Changes) + " GPIO13=" + String(led2Changes));
          Serial.println("\nconfig.h settings:");
          Serial.println("  #define MMDVM_RX_PIN " + String(workingRX));
          Serial.println("  #define MMDVM_TX_PIN " + String(workingTX));
          Serial.println("  #define MMDVM_SERIAL_BAUD " + String(workingBaud));

          // Continue to find all working configs
          Serial.println("\nContinuing scan...\n");
        }
        
        delay(50);
        yield();  // Feed watchdog
      }
    }
  }

  Serial.println("\n══════════════════════════════════════════════════════");
  Serial.println("                 SCAN COMPLETE");
  Serial.println("══════════════════════════════════════════════════════\n");

  Serial.println("Total tests performed: " + String(testCount));
  Serial.println("LED activity detected: GPIO12=" + String(led1Changes) + " GPIO13=" + String(led2Changes));
  
  if (foundWorking) {
    Serial.println("\n✓ Working configuration found!");
    Serial.println("\nRECOMMENDED CONFIG:");
    Serial.println("  RX Pin:    GPIO " + String(workingRX));
    Serial.println("  TX Pin:    GPIO " + String(workingTX));
    Serial.println("  Baud Rate: " + String(workingBaud));
  } else {
    Serial.println("\n✗ No working configuration found\n");
    Serial.println("Troubleshooting:");
    Serial.println("1. Check MMDVM power LED is lit");
    Serial.println("2. Verify MMDVM firmware is flashed");
    Serial.println("3. Check physical connections");
    Serial.println("4. Verify ground connection ESP32 ↔ MMDVM");
    Serial.println("5. Try different MMDVM firmware");
    
    if (led1Changes > 0 || led2Changes > 0) {
      Serial.println("\n⚠ LED activity detected - MMDVM is responding!");
      Serial.println("  but UART communication failed.");
      Serial.println("  GPIO 12/13 are MMDVM status outputs, not UART.");
    }
  }

  Serial.println("\n══════════════════════════════════════════════════════\n");
}

void loop() {
  // Flash LED if found working config
  if (foundWorking) {
    Serial.print(".");
    delay(1000);
  } else {
    delay(5000);
  }
}

bool testConfiguration(PinPair pins, uint32_t baud, int testNum) {
  // Configure serial
  TestSerial.end();
  delay(30);

  // Begin with these pins
  TestSerial.begin(baud, SERIAL_8N1, pins.rx, pins.tx);
  delay(80);

  // Flush any old data
  while (TestSerial.available()) {
    TestSerial.read();
  }
  delay(30);

  // Reset LED counters
  unsigned long led1Before = led1Changes;
  unsigned long led2Before = led2Changes;

  // Send GET_VERSION command
  uint8_t cmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};
  size_t written = TestSerial.write(cmd, 3);

  if (written != 3) {
    return false;
  }

  TestSerial.flush();

  // Compact output
  Serial.print("  [" + String(testNum) + "] ");
  Serial.print("RX" + String(pins.rx) + "/TX" + String(pins.tx) + "@");
  
  // Shorten baud display
  if (baud >= 100000) {
    Serial.print(String(baud / 1000) + "k");
  } else {
    Serial.print(String(baud));
  }
  Serial.print(" -> ");

  // Wait for response
  delay(120);
  bool gotResponse = false;
  int byteCount = 0;
  uint8_t firstByte = 0;
  uint8_t rxBuffer[50];

  // Check for response
  for (int check = 0; check < 6; check++) {
    delay(15);
    while (TestSerial.available() && byteCount < 50) {
      uint8_t b = TestSerial.read();
      rxBuffer[byteCount] = b;
      if (byteCount == 0) firstByte = b;
      gotResponse = true;
      byteCount++;
    }
    if (gotResponse) break;
  }

  // Check LED activity
  unsigned long led1Activity = led1Changes - led1Before;
  unsigned long led2Activity = led2Changes - led2Before;
  bool ledActivity = (led1Activity > 0 || led2Activity > 0);

  if (!gotResponse) {
    if (ledActivity) {
      Serial.println("no UART (LED activity: " + String(led1Activity + led2Activity) + ")");
    } else {
      Serial.println("silent");
    }
    return false;
  }

  // Show received data (first few bytes)
  for (int i = 0; i < min(byteCount, 8); i++) {
    if (rxBuffer[i] < 0x10) Serial.print("0");
    Serial.print(rxBuffer[i], HEX);
    Serial.print(" ");
  }

  // Analyze response
  if (firstByte == MMDVM_FRAME_START) {
    Serial.print(" ✓✓✓ VALID!");
    if (ledActivity) {
      Serial.print(" (LED:" + String(led1Activity + led2Activity) + ")");
    }
    Serial.println();
    return true;
  } else {
    Serial.print(" (invalid)");
    if (ledActivity) {
      Serial.print(" LED:" + String(led1Activity + led2Activity));
    }
    Serial.println();
    return false;
  }
}
