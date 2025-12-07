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

### 1. Startup Sequence
On power-up, the OLED displays a professional boot sequence:

**ESP32 Splash Screen (2 seconds):**
- Large "ESP32" logo with decorative borders
- "MMDVM Hotspot" subtitle
- Logged to serial: "OLED: Showing ESP32 logo"

**Boot Logo with Live Status Updates:**
- **Centered callsign header** (e.g., "PD2EMC - ESP32 HS")
- **Firmware version**
- **Authors** (PD2EMC & PD8JO)
- **Live boot progress** - Status line updates in real-time:
  - "Booting..."
  - "Loading config..."
  - "Init MMDVM..."
  - "Init SD Card..."
  - "Connecting ETH..." or "Connecting WiFi..."
  - "Starting web..."
  - "Syncing time..."
  - "Connecting DMR..."
  - "Ready!"
- All status updates logged to serial monitor

### 2. Network Status Display
After boot, the display shows:
- **WiFi/Ethernet connection status** with IP address
- **Automatic switching** between WiFi and Ethernet (when both connected)
- **AP mode fallback** display
- **Independent 5-second timer** - Network display toggles smoothly every 5 seconds regardless of DMR activity

### 3. Real-Time DMR Activity Display
The OLED shows live DMR transmissions:

**When idle (no transmission):**
- "DMR: Listening"
- Current talkgroup (TG) number

**During active transmission:**
- **Slot indicator** - [S1] or [S2]
- **Callsign** - Source station callsign
- **DMR ID → Talkgroup** - Complete routing info
- **Duration** - Real-time transmission duration in seconds
- **Fast refresh** - Updates every 1 second during active transmissions
- **Smart slot switching** - When both slots active, alternates display every second

### Display Layout

```
┌────────────────────────┐
│  PD2EMC - ESP32 HS     │ ← Centered header
├────────────────────────┤
│                        │
│      PA0ABC            │ ← LARGE callsign (2x, centered)
│                        │
│ Duration: 5s           │ ← Duration
│ 2043999->TG 91 [S2]    │ ← DMR ID→TG + Slot
├────────────────────────┤
│ WiFi: 192.168.1.50     │ ← Network status
└────────────────────────┘
```

## Future Enhancements (Planned)

Additional features that can be added:
- **Signal quality** - BER/RSSI display (requires MMDVM firmware support)
- **System Info** - Uptime, memory usage, CPU temperature
- **Multi-page Display** - Rotate through different info screens
- **Auto-dimming** - Reduce brightness when idle
- **User name display** - Show operator name from DMR database

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

## Technical Details

### I2C Bus Sharing
The I2C pins (GPIO 17/18) can be shared with other I2C devices. Each device needs a unique I2C address.

### Display Refresh Rate
The boot logo is displayed once during startup. Future updates will add periodic screen updates for real-time information.

### Memory Usage
- Display buffer: ~1KB RAM (128x64 bits)
- Minimal impact on ESP32's 520KB RAM

## Support

For issues or questions:
- Check the serial monitor for OLED debug messages
- Verify wiring with I2C scanner
- Ensure correct I2C address in config.h
- Report issues on GitHub

---

**Enjoy your OLED-enhanced MMDVM hotspot!** 🎉
