# ESP32 MMDVM Hotspot - Factory Setup

Minimal bootstrap firmware for initial deployment and mass production of ESP32 MMDVM Hotspot devices.

## Overview

The factory setup firmware is a lightweight, minimal firmware designed to be flashed onto fresh ESP32 devices. It provides a simple web interface for configuring WiFi and deploying the full MMDVM firmware via OTA (Over-The-Air) update.

## Features

- **Automatic WiFi Connection**: Attempts to connect to a default WiFi network or creates an Access Point
- **WiFi Network Scanner**: Scan and select from available WiFi networks with signal strength indicators
- **WiFi Credential Storage**: Saves WiFi credentials to NVS (Non-Volatile Storage) for persistence
- **OTA Firmware Deployment**: Download and flash stable or beta MMDVM firmware directly from GitHub
- **Manual Firmware Upload**: Upload custom firmware files (.bin) for offline deployment
- **Dark/Light Theme**: Manual theme toggle with auto-detection and localStorage persistence
- **Clean Storage**: Uses the same "mmdvm" namespace as main firmware for easy cleanup

## Use Cases

1. **Mass Production**: Flash this firmware on new ESP32 devices before shipping to end users
2. **Factory Reset**: Recover devices that have configuration issues
3. **Field Deployment**: Easy setup for ham radio operators without technical expertise
4. **Development**: Quick deployment of firmware updates during development

## Configuration

Edit `config.h` to customize the factory setup:

```cpp
// Default WiFi (device will try to connect on first boot)
#define WIFI_SSID "YourWiFi"
#define WIFI_PASSWORD "YourPassword"

// Access Point (created if default WiFi fails)
#define AP_SSID "ESP32-MMDVM-SETUP"
#define AP_PASSWORD "mmdvm1234"

// OTA URLs (points to your firmware repository)
#define OTA_UPDATE_URL "https://raw.githubusercontent.com/your-repo/main/update.bin"
#define OTA_UPDATE_BETA_URL "https://raw.githubusercontent.com/your-repo/main/update_beta.bin"
```

## Flashing Factory Firmware

### Using Arduino IDE

1. Open `factory-setup.ino` in Arduino IDE
2. Select your ESP32 board (e.g., ESP32 Dev Module)
3. Configure partition scheme: **Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)**
4. Connect ESP32 via USB
5. Click Upload

### Using PlatformIO

```ini
[env:factory]
platform = espressif32
board = esp32dev
framework = arduino
board_build.partitions = min_spiffs.csv
```

Then run:
```bash
pio run -e factory -t upload
```

### Using esptool.py

```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash 0x10000 factory-setup.bin
```

## User Workflow

### Scenario 1: Default WiFi Available

1. **Power On**: ESP32 boots with factory firmware
2. **Auto-Connect**: Connects to default WiFi from config.h
3. **Access Web Interface**: Navigate to device IP (shown in serial output)
4. **Deploy Firmware**:
   - Select firmware version (Stable or Beta)
   - Click "Download & Flash Firmware"
   - Wait for download and installation
5. **Reboot**: Device restarts with full MMDVM firmware

### Scenario 2: Default WiFi Not Available (Access Point Mode)

1. **Power On**: ESP32 boots with factory firmware
2. **AP Mode**: Creates Access Point (ESP32-MMDVM-SETUP)
3. **Connect**: User connects to AP with password `mmdvm1234`
4. **Open Browser**: Navigate to `http://192.168.4.1`
5. **Configure WiFi**:
   - Click "📡 Scan for Networks"
   - Select your WiFi network from the list
   - Enter WiFi password
   - Click "Connect to WiFi"
6. **Auto-Redirect**: Page redirects to new IP address
7. **Deploy Firmware**:
   - Select firmware version (Stable or Beta)
   - Click "Download & Flash Firmware"
   - Wait for download and installation
8. **Reboot**: Device restarts with full MMDVM firmware

### Scenario 3: Manual Firmware Upload (Offline)

1. Follow steps 1-6 from Scenario 2 (if WiFi config needed)
2. **Upload Firmware**:
   - Click "Upload File" under firmware deployment
   - Select your .bin firmware file
   - Click "Upload Firmware"
   - Wait for upload (progress bar shown)
   - Confirm flash when prompted
3. **Reboot**: Device restarts with uploaded firmware

## Web Interface

The factory setup provides a clean, modern web interface with the following sections:

### Network Status Card
- Shows current connection status (WiFi or AP mode)
- Displays connected network name and IP address
- Internet connection indicator

### WiFi Configuration Card (AP Mode Only)
- Network scanner with signal strength and encryption indicators
- Manual SSID entry option
- Password input field
- One-click connection

### System Information Card
- Factory firmware version
- ESP32 chip model
- Flash size
- Free heap memory

### Firmware Deployment Card
- Online firmware version checker (Stable & Beta)
- Version selection dropdown
- Online update button
- Manual file upload option
- Progress indicators
- Automatic flash confirmation

## Storage and Persistence

### WiFi Credentials
- Stored in NVS namespace: `"mmdvm"`
- Keys: `"ssid"` and `"password"`
- Shared with main MMDVM firmware
- Persists across reboots and firmware updates

### Theme Preference
- Stored in browser localStorage
- Key: `"theme"`
- Values: `"dark"` or `"light"`
- Persists per browser/device

### ESP32 Internal WiFi Flash
- Cleared on every boot to prevent ghost connections
- Only Preferences-stored credentials are used

## Serial Debugging

Connect to serial port at 115200 baud to see debug output:

```
===========================================
ESP32 MMDVM Hotspot - Factory Setup
Version: FACTORY_SETUP_v1.0
===========================================

Found saved WiFi credentials in Preferences
Attempting to connect to saved WiFi: MyNetwork
.....
WiFi connected!
IP address: 192.168.1.100

Web server started!
Navigate to: http://192.168.1.100

Ready for OTA firmware update.
```

## Troubleshooting

### Device won't connect to WiFi
- Check SSID and password are correct
- Verify WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Try scanning for networks to ensure network is visible
- Check serial output for connection attempts

### Can't access web interface
- Verify you're connected to the same network as ESP32
- Check IP address in serial output
- Try `http://esp32-mmdvm.local` if mDNS is enabled
- In AP mode, ensure you're connected to ESP32's AP

### Firmware download fails
- Verify ESP32 has internet connection
- Check GitHub URLs are accessible
- Try manual upload instead
- Check serial output for HTTP error codes

### Upload times out
- Use smaller firmware files if possible
- Ensure stable connection to ESP32
- Try reducing distance to ESP32 if using WiFi
- Check available heap memory in system info

### Wrong WiFi network displayed
- Clear NVS storage from main firmware admin panel
- Or reflash factory firmware (this clears WiFi credentials)
- Check saved credentials in serial output on boot

## Technical Details

### Memory Usage
- Minimal footprint: ~300KB program size
- Small web interface (HTML/CSS/JS embedded)
- Efficient string handling for large files
- OTA partition support required

### Network Features
- WiFi STA mode for internet connectivity
- WiFi AP mode for initial setup
- Automatic fallback between modes
- DNS resolution for GitHub URLs
- HTTP client for firmware downloads

### Security Considerations
- WiFi passwords stored in NVS (encrypted by ESP32)
- No external authentication required
- AP mode uses WPA2 encryption
- Firmware verification before flashing
- No sensitive data transmission

### Compatibility
- Works with all ESP32 variants (ESP32, ESP32-S2, ESP32-S3, ESP32-C3)
- Compatible with Arduino IDE and PlatformIO
- Requires partition scheme with OTA support
- Minimum 2MB flash recommended

## File Structure

```
factory-setup/
├── factory-setup.ino    # Main sketch
├── config.h             # Configuration settings
├── webpages.h          # Web interface HTML/CSS/JS
└── README.md           # This file
```

## Updating Factory Firmware

To update the factory firmware itself:

1. Make changes to source files
2. Update `FACTORY_VERSION` in config.h
3. Compile and flash via USB
4. Test WiFi configuration and OTA deployment
5. Document any changes

## Credits

Created by **PD2EMC & PD8JO**

- Website: [einstein.amsterdam](https://einstein.amsterdam) | [pd8jo.nl](https://pd8jo.nl)
- GitHub: [esp32_mmdvm_hotspot](https://github.com/javastraat/esp32_mmdvm_hotspot)

## License

Same as main MMDVM firmware - check main repository for details.

## Support

For issues or questions:
- GitHub Issues: [Create an issue](https://github.com/javastraat/esp32_mmdvm_hotspot/issues)
- Serial debugging output is your friend!

---

**Happy Deploying! 73 de PD2EMC & PD8JO**
