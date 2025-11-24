# ESP32 MMDVM Hotspot - Setup Guide

## Overview
This project turns an ESP32 into a DMR hotspot using an MMDVM hat, similar to Pi-Star functionality.

## Hardware Requirements

### Essential Components
1. **ESP32 Development Board** (ESP32-WROOM-32 or similar)
2. **MMDVM Hat** (supports DMR, D-STAR, YSF, P25, etc.)
3. **Radio Module** (usually included with MMDVM hat)
4. **Antenna** (appropriate for your frequency)
5. **Level Shifters** (if MMDVM hat is 5V logic)

### Recommended Boards
- ESP32 DevKit V1
- ESP32-WROOM-32
- ESP32-S3 (for more advanced features)

## Wiring Diagram

```
ESP32          MMDVM Hat
------         ----------
GPIO16 (RX2) -> TX
GPIO17 (TX2) -> RX
GPIO4        -> PTT
GPIO2        -> COS/LED
3.3V         -> VCC (or 5V with level shifter)
GND          -> GND
```

**Important:** Most MMDVM hats expect 5V logic levels. If using 3.3V ESP32:
- Use bidirectional level shifters for RX/TX lines
- Or use a 3.3V compatible MMDVM hat
- Or modify MMDVM hat for 3.3V operation

## Software Setup

### 1. Install Arduino IDE
Download from: https://www.arduino.cc/en/software

### 2. Install ESP32 Board Support
1. Open Arduino IDE
2. Go to File -> Preferences
3. Add to "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to Tools -> Board -> Board Manager
5. Search for "ESP32" and install "ESP32 by Espressif Systems"

### 3. Configure the Code

Edit the sample code and update these settings:

```cpp
// WiFi Settings
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// DMR Network Settings
const char* dmr_server = "44.131.4.1";  // BrandMeister server
const uint32_t dmr_id = 1234567;         // Your DMR ID
const char* dmr_callsign = "N0CALL";     // Your callsign
const char* dmr_password = "passw0rd";   // Your hotspot password
```

### 4. Get Your DMR Credentials

**BrandMeister:**
1. Go to https://brandmeister.network/
2. Register with your callsign
3. Get your DMR ID
4. Set up hotspot security password in your BrandMeister profile

**DMRplus:**
1. Go to https://dmrplus.org/
2. Register and get credentials

### 5. Upload the Code
1. Select board: Tools -> Board -> ESP32 Dev Module
2. Select correct COM port
3. Click Upload

## Pin Configuration

You can modify the GPIO pins in the code if needed:

```cpp
#define PTT_PIN 4        // Push-to-talk control
#define COS_LED_PIN 2    // Carrier detect LED
#define RX_PIN 16        // Serial receive
#define TX_PIN 17        // Serial transmit
```

## DMR Network Servers

### BrandMeister Servers
- US: `44.131.4.1:62030`
- EU: `44.225.62.11:62030`
- Australia: `45.248.50.1:62030`

### DMRplus Servers
- Check https://www.dmrplus.org/ for current servers

## Troubleshooting

### MMDVM Not Responding
1. Check serial wiring (RX/TX might be swapped)
2. Verify baud rate matches MMDVM firmware (usually 115200)
3. Check voltage levels (3.3V vs 5V)
4. Try adding level shifters

### WiFi Connection Issues
1. Verify SSID and password
2. Check 2.4GHz WiFi is enabled (ESP32 doesn't support 5GHz)
3. Move closer to router during initial setup

### No Network Connection
1. Verify DMR server address and port
2. Check firewall settings (UDP port 62030-62031)
3. Verify DMR ID and password are correct
4. Check BrandMeister website to ensure your hotspot is registered

### MMDVM Hat Not Transmitting
1. Check PTT pin connection
2. Verify antenna is connected
3. Check frequency settings match your hardware
4. Ensure proper RF grounding

## Advanced Configuration

### Adjusting RF Levels
Modify in the config section:
```cpp
config[5] = 0x80;  // RX Level (0x00-0xFF, 0x80 = 50%)
config[6] = 0x80;  // TX Level (0x00-0xFF, 0x80 = 50%)
```

### Changing Frequency
The frequency is typically set on the MMDVM hardware. For software-defined versions:
```cpp
// Add frequency setting command
uint32_t freq = 434000000; // 434 MHz in Hz
sendMMDVMCommand(CMD_SET_FREQ, (uint8_t*)&freq, 4);
```

## Monitoring and Debugging

### Serial Monitor
Open Serial Monitor at 115200 baud to see:
- WiFi connection status
- MMDVM initialization
- Received/transmitted packets
- Network keepalive status

### LED Indicators
- COS LED blinks when receiving DMR traffic
- Built-in LED can be used for status indication

## Next Steps

1. **Add Display:** Connect an OLED display (I2C) to show status
2. **Web Interface:** Add a web server for configuration
3. **Multi-Protocol:** Enable D-STAR, YSF, P25 support
4. **GPS:** Add GPS module for APRS functionality
5. **Power Management:** Implement sleep modes for battery operation

## Protocol Implementation Notes

The sample code provides a basic framework. For full functionality, you'll need to implement:

1. **Complete DMR Protocol:**
   - Proper packet framing
   - Slot timing
   - Color code handling
   - Talkgroup routing

2. **Network Protocols:**
   - Full BrandMeister/DMRplus login sequence
   - Packet acknowledgments
   - Master server selection

3. **MMDVM Modem Control:**
   - Calibration procedures
   - Mode switching (DMR/D-STAR/etc.)
   - Error handling

## Resources

- **MMDVM GitHub:** https://github.com/g4klx/MMDVM
- **MMDVMHost:** https://github.com/g4klx/MMDVMHost
- **BrandMeister:** https://brandmeister.network/
- **Pi-Star:** https://www.pistar.uk/
- **ESP32 Documentation:** https://docs.espressif.com/

## License
This sample code is provided as-is for educational purposes.

## Safety and Legal Warnings

⚠️ **Important:**
- Ensure you have proper amateur radio license for your region
- Operate within legal frequency bands
- Use appropriate RF shielding
- Never transmit without proper antenna
- Follow local regulations for RF power levels

## Contributing
Feel free to improve this code and share your modifications!

## Support
For issues specific to:
- MMDVM hardware: Check MMDVM forums
- DMR networks: Contact your chosen network support
- ESP32: Check ESP32 Arduino community
# esp32_mmdvm_hotspot
