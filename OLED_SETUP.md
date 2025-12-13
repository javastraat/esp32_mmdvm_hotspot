# OLED Display Setup Guide

## Overview
The ESP32 MMDVM Hotspot now supports an optional SSD1306 OLED display for real-time status information.

## Hardware Requirements

### Display Specifications
- **Model**: SSD1306 OLED Display
- **Resolution**: 128x64 pixels
- **Interface**: I2C
- **I2C Address**: 0x3C (default) or 0x3D
- **Voltage**: 3.3V or 5V compatible (use 3.3V with ESP32)

### Where to Buy
- Amazon: Search "SSD1306 128x64 OLED I2C"
- AliExpress: Search "0.96 inch OLED display I2C"
- Adafruit: Product ID 326 (Monochrome 128x64)
- SparkFun: Product COM-14532

**Cost**: Typically $3-$10 USD

## Wiring Connection

Connect the OLED display to your ESP32:

```
OLED Display    ESP32 GPIO    Function
------------    ----------    --------
VCC             3.3V          Power
GND             GND           Ground
SCL             GPIO 18       I2C Clock
SDA             GPIO 17       I2C Data
```

**Important Notes:**
- Use 3.3V power, NOT 5V (ESP32 is 3.3V logic)
- Most SSD1306 displays have built-in pull-up resistors on SDA/SCL
- GPIO 17 and 18 are shared I2C pins defined in config.h

## Software Setup

### 1. Install Required Libraries

Using Arduino IDE Library Manager:

1. Open Arduino IDE
2. Go to **Sketch → Include Library → Manage Libraries**
3. Search for "**Adafruit GFX**" and click **Install**
4. Search for "**Adafruit SSD1306**" and click **Install**

### 2. Enable OLED in Configuration

The OLED is already enabled by default in `config.h`:

```cpp
// OLED Display Settings
#define ENABLE_OLED true       // Enable OLED display support
#define OLED_I2C_ADDRESS 0x3C  // I2C address (0x3C or 0x3D)
#define OLED_WIDTH 128         // Display width
#define OLED_HEIGHT 64         // Display height
```

### 3. I2C Address Configuration

Most SSD1306 displays use address **0x3C**, but some use **0x3D**.

**To find your display's I2C address:**

Use this I2C scanner sketch:
```cpp
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin(17, 18); // SDA=17, SCL=18
  Serial.println("\nI2C Scanner");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Serial.println("Scanning...");
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
  }

  if (nDevices == 0) Serial.println("No I2C devices found");
  delay(5000);
}
```

If your display uses **0x3D**, update `config.h`:
```cpp
#define OLED_I2C_ADDRESS 0x3D
```

## Current Features

### Display Layout

The OLED display uses a modern three-section layout:

```
┌─────────────────────────────────────┐
│ 13/12  14:23      [WiFi][ETH][ANT] │ ← Date/Time + Status Icons
├─────────────────────────────────────┤
│                                     │
│           PA0ABC                    │ ← LARGE callsign (2x)
│                                     │
│ Duration: 5s                        │ ← Duration
│ 2043999 -> TG 91 [S2]               │ ← DMR ID→TG + Slot
├─────────────────────────────────────┤
│ PD2EMC - ESP32 HS                   │ ← Bottom cycling info
└─────────────────────────────────────┘
```

### 1. Top Status Bar (Line 1)

**Left Side - Date:**
- Current date in DD/MM format (e.g., "13/12")
- Synced from NTP server

**Center - Time:**
- Current time in HH:MM format (e.g., "14:23")
- 24-hour format
- Shows "--:--" if time not synced

**Right Side - Status Icons (8x8 pixel icons):**
- **WiFi Icon** - Shows when WiFi connected
- **Ethernet Icon** - Shows when Ethernet connected (T-ETH-Elite only)
- **Antenna/RF Icon** - Shows when DMR mode enabled and logged in
- Icons displayed right-to-left with 2-pixel spacing
- Both WiFi and Ethernet icons shown simultaneously when both connected

### 2. Middle Activity Section (Lines 2-5)

**No Mode Activated:**
- Displays centered messages:
  - "No mode activated"
  - "Enable mode in web"

**DMR Listening (No Active Transmission):**
- "DMR: Listening"
- Current talkgroup: "TG: 91" (if available)
- Last caller: "Last: PA0ABC" (most recent from either slot)

**DMR Not Connected:**
- "DMR Mode Active"
- "Status: Connecting..." (shows current login status)

**Active DMR Transmission:**
- **Large Callsign** - 2x size text, centered (e.g., "PA0ABC")
- **Duration** - Live counter in seconds (e.g., "Duration: 5s")
- **Routing Info** - DMR ID → TG with slot (e.g., "2043999 -> TG 91 [S2]")
- **Fast Refresh** - Updates every 1 second during activity
- **Dual-Slot Support:**
  - Prioritizes Slot 2 when only one active
  - Alternates between slots every second when both active
  - Smooth slot switching with `oledActiveSlot` toggle

**Other Modes (D-Star, YSF, P25, NXDN, POCSAG):**
- Shows "Mode enabled:"
- Displays mode name with "(N/A)" indicator

### 3. Bottom Information Bar (Line 6)

**Cycling Display** (cycles every 5 seconds based on `oledHeaderCycle`):

**When WiFi + Ethernet Both Connected (3 screens):**
1. "WiFi: 192.168.1.50" (left-aligned)
2. "ETH: 192.168.1.100" (left-aligned)
3. "PD2EMC - ESP32 HS" (centered)

**When WiFi Only (2 screens):**
1. "WiFi: 192.168.1.50" (left-aligned)
2. "PD2EMC - ESP32 HS" (centered)

**When Ethernet Only (2 screens):**
1. "ETH: 192.168.1.100" (left-aligned)
2. "PD2EMC - ESP32 HS" (centered)

**When AP Mode (2 screens):**
1. "AP: 192.168.4.1" (left-aligned)
2. "PD2EMC - ESP32 HS" (centered)

**No Network (2 screens):**
1. "No Network" (centered)
2. "PD2EMC - ESP32 HS" (centered)

### 4. Update Intervals

- **Active DMR Transmission:** 1 second refresh (`OLED_UPDATE_INTERVAL_ACTIVE`)
- **Idle/Listening:** 5 second refresh (`OLED_UPDATE_INTERVAL`)
- **Bottom Bar Cycling:** 5 seconds (`NETWORK_TOGGLE_INTERVAL`)
- **Independent Timers:** Network cycling is independent from DMR activity updates

### 5. Thread Safety Features

**Mutex Protection:**
- Display updates protected by `displayMutex` semaphore
- Prevents SPI conflicts with Ethernet hardware
- 50ms timeout - skips update if mutex unavailable
- Logged message when update skipped: "OLED: Skipped update - mutex busy"

**Clean Updates:**
- Full `clearDisplay()` before each update
- Complete redraw ensures no artifacts
- All text properly bounded and centered

## Technical Implementation Details

### Icon Definitions
The firmware includes three 8x8 pixel bitmap icons defined in `esp32_mmdvm_hotspot.ino`:

```cpp
#define ICON_WIDTH 8
#define ICON_HEIGHT 8

// WiFi icon - wireless signal waves
static const unsigned char PROGMEM icon_wifi[] = { ... };

// Ethernet icon - cable/port symbol
static const unsigned char PROGMEM icon_ethernet[] = { ... };

// Antenna/RF icon - radio wave pattern
static const unsigned char PROGMEM icon_antenna[] = { ... };
```

### Display Function
The main update function is `updateOLEDStatus()` which:
1. Acquires display mutex (50ms timeout)
2. Clears entire display
3. Draws top status bar (date/time/icons)
4. Draws horizontal line separator
5. Updates middle activity section based on mode/state
6. Draws bottom horizontal line separator
7. Updates bottom cycling information bar
8. Calls `display.display()` to render
9. Releases display mutex

### Configuration Constants
```cpp
#define OLED_UPDATE_INTERVAL 5000        // Idle refresh: 5 seconds
#define OLED_UPDATE_INTERVAL_ACTIVE 1000 // Active refresh: 1 second
#define NETWORK_TOGGLE_INTERVAL 5000     // Bottom bar cycle: 5 seconds
```

### Global Variables
```cpp
SemaphoreHandle_t displayMutex          // Thread safety mutex
unsigned long lastOLEDUpdate            // Last update timestamp
bool oledShowEthernet                   // ETH/WiFi toggle state
int oledActiveSlot                      // Active slot (0 or 1)
int oledHeaderCycle                     // Bottom bar cycle counter
```

## Future Enhancements (Planned)

Additional features that can be added:
- **Signal quality** - BER/RSSI display (requires MMDVM firmware support)
- **System Info Page** - Uptime, memory usage, CPU temperature
- **Brightness Control** - Auto-dimming when idle
- **User name display** - Show operator name from RadioID.net database
- **Battery Voltage** - For portable operation monitoring
- **Custom Layouts** - User-configurable display preferences

## Troubleshooting

### Display Not Working

1. **Check Wiring**
   - Verify VCC → 3.3V (NOT 5V!)
   - Verify GND → GND
   - Verify SDA → GPIO 17
   - Verify SCL → GPIO 18

2. **Check I2C Address**
   - Run I2C scanner sketch (above)
   - Update `OLED_I2C_ADDRESS` in config.h if needed

3. **Check Serial Monitor**
   - Look for "OLED: Display initialized successfully"
   - If you see "OLED: SSD1306 allocation failed!" - wiring issue or wrong address

4. **Verify Libraries**
   - Ensure Adafruit GFX and Adafruit SSD1306 are installed
   - Try reinstalling libraries if compilation fails

### Display Shows Garbage/Random Pixels

- Wrong I2C address (try 0x3D instead of 0x3C)
- Loose wiring connection
- Bad display module (rare)

### Compilation Errors

**Error: Wire.h not found**
- You need to install ESP32 board support in Arduino IDE

**Error: Adafruit_GFX.h not found**
- Install "Adafruit GFX Library" via Library Manager

**Error: Adafruit_SSD1306.h not found**
- Install "Adafruit SSD1306" via Library Manager

## Disabling the OLED

To disable the OLED display, edit `config.h`:

```cpp
#define ENABLE_OLED false
```

The code will compile without the Adafruit libraries when disabled.

## Performance & Resources

### I2C Bus Sharing
The I2C pins (GPIO 17/18) can be shared with other I2C devices. Each device needs a unique I2C address.

### Display Refresh Rate
- **Idle State:** 5 seconds between updates
- **Active DMR:** 1 second refresh for live duration counter
- **Bottom Bar:** Independent 5-second cycling regardless of DMR activity
- **Mutex-Protected:** Skip update if SPI bus busy (Ethernet conflicts)

### Memory Usage
- **Display Buffer:** ~1KB RAM (128x64 bits monochrome)
- **Icon Bitmaps:** 24 bytes (3 icons × 8 bytes each)
- **Minimal Impact:** <0.2% of ESP32's 520KB RAM

### Thread Safety
- Uses FreeRTOS semaphore (`displayMutex`) to prevent conflicts
- 50ms timeout prevents blocking main loop
- Safe concurrent access with Ethernet SPI operations
- Logs "OLED: Skipped update - mutex busy" if timeout occurs

## Support

For issues or questions:
- Check the serial monitor for OLED debug messages
- Verify wiring with I2C scanner
- Ensure correct I2C address in config.h
- Report issues on GitHub

---

**Enjoy your OLED-enhanced MMDVM hotspot!** 🎉
