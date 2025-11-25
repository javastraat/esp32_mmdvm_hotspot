# ESP32 MMDVM Hotspot

A comprehensive ESP32-based DMR hotspot with web interface, WiFi management, and BrandMeister network integration.

## 🌟 Features

- **Full DMR Protocol Support** - Complete BrandMeister network integration with authentication
- **Advanced Web Interface** - Configuration and monitoring via responsive web browser
- **Dual WiFi Support** - Primary network from config.h + alternate network via web interface
- **Real-time Serial Monitor** - Live MMDVM communication logs via web interface
- **Responsive Design** - Mobile-friendly web interface with navigation menu
- **Configuration Storage** - Persistent settings stored in ESP32 flash memory
- **Network Scanner** - WiFi network discovery and configuration
- **DMR Status Monitoring** - Real-time connection status and network information
- **Show Preferences** - View all ESP32 stored settings for debugging
- **Complete Factory Reset** - Erase entire ESP32 NVS partition for troubleshooting

## 🔧 Hardware Requirements

### Essential Components
1. **ESP32 Development Board** (ESP32-WROOM-32 recommended)
2. **MMDVM Hat** (JumboSPOT, MMDVM_HS, or compatible)
3. **Antenna** (UHF/VHF as appropriate for your frequency)

### Supported MMDVM Hardware
- JumboSPOT (recommended)
- MMDVM_HS (Hotspot)
- ZUMspot
- OpenSPOT compatible devices

## 📋 Pin Configuration

```
ESP32 GPIO    MMDVM Hat     Function
----------    ---------     --------
GPIO16 (RX2)  -> TX         MMDVM Serial TX
GPIO17 (TX2)  -> RX         MMDVM Serial RX  
GPIO4         -> PTT        Push-to-Talk Control
GPIO2         -> COS/LED    Carrier Detect LED
3.3V          -> VCC        Power Supply
GND           -> GND        Ground
```

**Note:** Most modern MMDVM hats (JumboSPOT, MMDVM_HS) are 3.3V compatible and work directly with ESP32.

## 🚀 Quick Start

### 1. Hardware Setup
1. Connect your MMDVM hat to ESP32 according to pin configuration
2. Connect antenna to MMDVM hat
3. Power the ESP32 via USB or external supply

### 2. Software Installation

#### Arduino IDE Setup
1. Download Arduino IDE from: https://www.arduino.cc/en/software
2. Install ESP32 board support:
   - File → Preferences
   - Add to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Tools → Board → Board Manager
   - Search "ESP32" and install "ESP32 by Espressif Systems"

#### Required Libraries
All required libraries are built into ESP32 Arduino Core:
- WiFi (WiFi network connectivity)
- WebServer (Web interface)  
- ESPmDNS (Network discovery)
- Preferences (Configuration storage)
- mbedtls (Cryptographic functions)

### 3. Configuration

#### Edit config.h
Update the configuration file with your settings:

```cpp
// WiFi Configuration (Primary Network)
#define WIFI_SSID "YourWiFiNetwork"
#define WIFI_PASSWORD "YourWiFiPassword"

// DMR Credentials  
#define DMR_CALLSIGN "N0CALL"        // Your amateur radio callsign
#define DMR_ID 1234567               // Your 7-digit DMR ID
#define DMR_PASSWORD "passw0rd"      // Hotspot password from BrandMeister
#define DMR_SERVER "44.131.4.1"     // BrandMeister server address

// Hardware Configuration (adjust if needed)
#define MMDVM_RX_PIN 16             // ESP32 RX pin
#define MMDVM_TX_PIN 17             // ESP32 TX pin  
#define MMDVM_PTT_PIN 4             // PTT control pin
#define MMDVM_COS_LED_PIN 2         // Status LED pin
```

### 4. Upload Firmware
1. Select: Tools → Board → "ESP32 Dev Module"
2. Select your COM port: Tools → Port
3. Click Upload button
4. Monitor serial output at 115200 baud

### 5. First Time Setup
After upload, the ESP32 will:
1. Try to connect to your configured WiFi
2. If WiFi fails, create access point: **ESP32-MMDVM-Config**
3. Connect to AP with password: **mmdvm1234**
4. Open browser to: **http://192.168.4.1**
5. Configure WiFi and DMR settings via web interface

## 🌐 Web Interface Features

Once connected, access the web interface at the ESP32's IP address:

### Main Dashboard
- **Quick Status** - WiFi, DMR, and MMDVM connection status
- **Station Information** - Callsign, DMR ID, location details  
- **Network Activity** - Current talkgroup, frequency settings
- **System Overview** - Firmware version, server connection

### WiFi Configuration
- **Primary Network** - Main WiFi configured in config.h file
- **Alternate Network** - Backup WiFi configured via web interface
- **Automatic Failover** - Tries alternate network if primary fails
- **Network Scanner** - Discover and select nearby WiFi networks
- **Access Point Mode** - Creates hotspot if both WiFi networks fail

### DMR Configuration  
- **BrandMeister Integration** - Built-in server list with 40+ servers worldwide
- **Credential Management** - Callsign, DMR ID, and password configuration
- **Frequency Settings** - RX/TX frequency, power, and color code
- **Location Setup** - GPS coordinates and site information

### Serial Monitor
- **Real-time Logging** - Live MMDVM communication display
- **Auto-refresh** - Updates every 2 seconds (configurable)
- **Log Management** - Clear logs, pause/resume monitoring
- **Debug Information** - Network packets, authentication status

### Admin Panel
- **System Control** - Restart system, complete factory reset
- **Configuration Export** - Backup settings to file
- **Stored Preferences Viewer** - Advanced debugging tool showing all ESP32 NVS storage
- **Complete Storage Reset** - Erase entire ESP32 NVS partition (all namespaces)
- **Service Management** - Restart services, clear logs, test MMDVM
- **Preference Cleanup** - Fix corrupted storage and restore defaults

## 📡 DMR Network Support

### BrandMeister Servers (Built-in)
The web interface includes 40+ pre-configured BrandMeister servers:
- **Europe:** Netherlands, Germany, UK, France, Italy, Spain
- **North America:** USA, Canada  
- **Asia-Pacific:** Australia, Japan, Thailand
- **Others:** South Africa, Brazil, New Zealand

### Custom Servers
- Add any DMR network server manually
- Supports BrandMeister, DMRplus, and other networks
- Configurable server address and port

## 🔧 Advanced Configuration

### Dual WiFi Configuration
```cpp
// Primary WiFi (configured in config.h)
#define WIFI_SSID "YourNetwork"
#define WIFI_PASSWORD "YourPassword"

// Alternate WiFi (configured via web interface)
// Stored in ESP32 Preferences as "alt_ssid" and "alt_password"
// Automatic failover if primary network unavailable
```

### DMR Authentication Flow
1. **RPTL** - Initial login with DMR ID
2. **RPTK** - SHA256 authentication with salt+password
3. **RPTC** - Configuration packet with station details
4. **RPTPING** - Keepalive every 5 seconds

## 🔍 Troubleshooting

### WiFi Issues
**Problem:** Can't connect to WiFi
- Check SSID and password in config.h or web interface
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Try moving closer to router during setup
- Use WiFi scanner in web interface to verify network visibility

**Problem:** Frequent WiFi disconnections  
- Check signal strength in WiFi Config page
- Configure alternate network for automatic failover
- Verify router stability and channel congestion

### DMR Network Issues
**Problem:** "Login Failed" or "MSTNAK" errors
- Verify DMR ID is exactly 7 digits
- Check hotspot password in BrandMeister profile
- Ensure callsign matches your amateur radio license
- Try different BrandMeister server from the list

**Problem:** Connects but shows "Not Connected"
- Check BrandMeister website for server status
- Verify UDP port 62031 is not blocked by firewall
- Check if DMR ID is registered on BrandMeister
- Monitor serial output for authentication details

### MMDVM Hardware Issues  
**Problem:** "MMDVM Not Ready"
- Check all wiring connections (RX/TX, PTT, power)
- Verify MMDVM hat has proper firmware
- Check 115200 baud rate setting
- Ensure MMDVM hat is powered correctly

**Problem:** No RF transmission/reception
- Verify antenna connection and SWR
- Check frequency settings match your hardware
- Ensure proper RF grounding
- Verify power and color code settings

### Web Interface Issues
**Problem:** Can't access web interface
- Check ESP32 IP address in serial monitor  
- Try: http://esp32-mmdvm.local (if mDNS working)
- Access via AP mode: http://192.168.4.1
- Clear browser cache and try different browser

## 📊 Monitoring and Status

### Serial Monitor (115200 baud)
```
=== ESP32 MMDVM Hotspot ===
Initializing...
MMDVM Serial initialized
Connecting to WiFi: YourNetwork
WiFi Connected!
IP Address: 192.168.1.100
MMDVM Initialized
Connecting to DMR Network...
Server: 44.131.4.1:62031
Login packet sent, ID: 1234567
Login ACK received, sending auth...
Auth ACK received, sending config...
Config ACK - CONNECTED!
DMR Network fully connected and operational!
```

### LED Status Indicators
- **COS LED (GPIO2):** Blinks during DMR traffic reception
- **Built-in LED:** Can indicate WiFi/network status
- **Web Interface:** Real-time status display

### Web Interface Status
- **Connection Status:** WiFi signal strength and IP address
- **DMR Status:** Login status, server connection, current talkgroup
- **MMDVM Status:** Hardware ready status, firmware version
- **Network Activity:** Packet counters, keepalive status

## ⚙️ Configuration Files

### config.h
Main configuration file with hardware and network settings:
```cpp
// WiFi Settings
#define WIFI_SSID "YourNetwork"
#define WIFI_PASSWORD "YourPassword"

// DMR Network  
#define DMR_CALLSIGN "N0CALL"
#define DMR_ID 1234567
#define DMR_SERVER "44.131.4.1"

// Hardware Pins
#define MMDVM_RX_PIN 16
#define MMDVM_TX_PIN 17
#define MMDVM_PTT_PIN 4
```

### webpages.h  
Web interface implementation with:
- HTML page generators
- CSS styling and responsive design
- JavaScript for interactive features
- REST API endpoints for configuration

### Persistent Storage
Settings stored in ESP32 flash memory (Preferences namespace: "mmdvm"):
- DMR credentials and server settings (callsign, ID, password, ESSID)
- Alternate WiFi network credentials
- RF parameters (frequencies, power, color code) and location data
- All settings viewable via "Show Preferences" admin function

## 🔧 Advanced Features

### Protocol Support
Currently implemented:
- **DMR (Digital Mobile Radio)** - Full BrandMeister network support

Future protocols (configurable in config.h but not yet implemented):
- D-STAR, YSF (System Fusion), P25, NXDN, POCSAG

### Network Protocols
- **BrandMeister:** Full authentication with SHA256 hashing (RPTL/RPTK/RPTC sequence)
- **Custom DMR Servers:** Manual server configuration via web interface

*Note: Currently optimized for BrandMeister. Other DMR networks may work but are not tested.*

### RF Configuration (Web Interface)
- **Frequency Settings:** RX/TX frequencies in Hz
- **Power Control:** 0-99% output power
- **Color Code:** 0-15 (typically 1 for BrandMeister)
- **ESSID:** Radio ID suffix (01-99)
- **Location:** GPS coordinates and site description

## 🛠️ Development and Customization

### Code Structure
```
esp32_mmdvm_hotspot/
├── esp32_mmdvm_hotspot.ino    # Main firmware with DMR BrandMeister protocol
├── config.h                   # Hardware and network configuration  
├── webpages.h                 # Complete web interface implementation
└── README.md                  # This comprehensive documentation
```

### Key Functions
- `setupWiFi()` - Dual WiFi connection with automatic failover
- `connectToDMRNetwork()` - BrandMeister authentication sequence  
- `handleMMDVMSerial()` - MMDVM communication protocol
- `handleNetwork()` - DMR packet processing and routing
- Web handlers in webpages.h for configuration interface

### Web Interface Pages
Actual implemented pages:
1. **Main Dashboard** (`/`) - System status and station information
2. **Status Page** (`/status`) - Detailed system metrics with auto-refresh
3. **Serial Monitor** (`/monitor`) - Real-time MMDVM communication logs
4. **WiFi Config** (`/config`) - Primary and alternate network configuration
5. **DMR Config** (`/dmrconfig`) - Complete DMR settings with 40+ server list
6. **Admin Panel** (`/admin`) - System control, preferences, and maintenance

### Factory Reset System
The admin panel includes comprehensive reset functionality:

**Complete Storage Reset:**
- Erases entire ESP32 NVS (Non-Volatile Storage) partition
- Uses `nvs_flash_erase()` for thorough flash memory wipe
- Clears ALL namespaces, not just application settings
- Removes any residual data from previous configurations
- Returns ESP32 to true factory state
- Useful for troubleshooting persistent configuration issues

**Safety Features:**
- Multiple confirmation warnings before execution
- Clear explanation of what will be lost
- Automatic reinitialization of NVS after erase
- Detailed logging of reset process

### Stored Preferences Viewer
Advanced debugging tool accessible via Admin Panel → "Show Preferences":

**Comprehensive Storage View:**
- Shows all 17 stored preferences in ESP32 NVS flash memory
- Displays data types (String, UInt32, Float, UChar, Int32)
- Shows storage size in bytes for each preference
- Real-time free heap memory statistics

**Security Features:**
- **Password Masking:** All password fields automatically hidden with asterisks
- **Interactive Toggle:** Click eye icon to reveal/hide actual password values
- **Type Identification:** Clearly marked "String (password)" type for security fields
- **Length Display:** Shows character count for password verification

**Technical Details:**
- **Namespace:** All preferences stored in 'mmdvm' namespace
- **Key Detection:** Automatically identifies all application keys
- **Corruption Detection:** Shows empty, corrupted, or missing preferences
- **Storage Efficiency:** Displays actual flash memory usage per setting

**Supported Preference Types:**
- **DMR Configuration:** Callsign, ID, server, password, frequencies, location
- **Network Settings:** Primary and alternate WiFi credentials  
- **Radio Parameters:** Power, color code, coordinates, antenna height
- **System Metadata:** Version info, operational parameters

Useful for troubleshooting configuration issues, verifying stored settings, and understanding ESP32 NVS storage utilization.

### Adding Features
1. **Additional Protocols:** Extend MMDVM packet handlers for D-STAR, YSF, P25
2. **Display Support:** Add OLED/LCD display integration via I2C
3. **OTA Updates:** Implement web-based firmware update system
4. **Extended WiFi Management:** Expand beyond current dual WiFi to multiple network slots
5. **Enhanced Logging:** Add persistent log storage with rotation and filtering

## 🌍 BrandMeister Server List

The web interface includes 40+ servers worldwide:

**Europe:** Netherlands (2041), Germany (2621/2622), UK (2341), France (2081/2082), Italy (2222), Spain (2141), Belgium (2061), Austria (2322), Switzerland (2282), Denmark (2382), Sweden (2402), Norway (2421), Finland (2441), Czech Republic (2302), Hungary (2162), Romania (2262), Poland (2602), Portugal (2682), Ireland (2721), Bulgaria (2841), Slovenia (2931), Russia (2502/2503), Ukraine (2551), Greece (2022)

**North America:** USA (3102/3103/3104), Canada (3021), Mexico (3341)

**Asia-Pacific:** Australia (5051), South Korea (4501), China (4602), Malaysia (5021), Philippines (5151)

**Middle East/Africa:** Israel (4251), South Africa (6551)

**South America:** Brazil (7242), Chile (7301)

All servers use standard BrandMeister port 62031.

## 📖 Resources and Documentation

### Official Resources
- **MMDVM Project:** https://github.com/g4klx/MMDVM
- **MMDVMHost:** https://github.com/g4klx/MMDVMHost  
- **BrandMeister Network:** https://brandmeister.network/
- **Pi-Star:** https://www.pistar.uk/
- **ESP32 Arduino:** https://github.com/espressif/arduino-esp32

### Hardware Vendors
- **JumboSPOT:** https://www.amateurwireless.com/
- **MMDVM_HS:** https://github.com/juribeparada/MMDVM_HS
- **ZUMspot:** https://www.zumspot.com/

### DMR Resources  
- **DMR-MARC Database:** https://www.dmr-marc.net/
- **RadioLabs ID Database:** https://radiolabs.com/
- **BrandMeister Dashboard:** https://brandmeister.network/

## ⚖️ Legal and Safety Information

### ⚠️ Amateur Radio License Required
- **Legal Operation:** Valid amateur radio license required in your country
- **Frequency Compliance:** Operate only within authorized amateur bands
- **Power Limits:** Respect maximum RF power regulations
- **Identification:** Proper station identification per local regulations

### 🔒 Network Ethics  
- **Server Resources:** Don't abuse network servers with excessive connections
- **Authentication:** Keep your hotspot password secure and unique
- **Updates:** Keep firmware updated for security and compatibility
- **Community:** Follow BrandMeister and network-specific guidelines

### 🛡️ RF Safety
- **Antenna Safety:** Use proper antenna with appropriate SWR
- **RF Exposure:** Follow SAR and MPE guidelines for your power level  
- **Grounding:** Ensure proper RF and electrical grounding
- **Environment:** Consider RF exposure to others in your area

## 🤝 Contributing

This project welcomes contributions! Areas for improvement:

1. **Protocol Extensions:** Additional DMR features, other digital modes
2. **Hardware Support:** New MMDVM variants, display modules  
3. **Web Interface:** Enhanced UI/UX, mobile optimization
4. **Documentation:** Setup guides, troubleshooting, translations
5. **Testing:** Different hardware combinations, network scenarios

### How to Contribute
1. Fork the repository
2. Create a feature branch
3. Test thoroughly with your hardware
4. Submit a pull request with clear description
5. Include any new dependencies or configuration requirements

## 📞 Support and Community

### Getting Help
- **Hardware Issues:** MMDVM forums and hardware vendor support
- **DMR Network:** BrandMeister support, network-specific forums
- **ESP32 Development:** ESP32 Arduino community, Espressif forums
- **General Amateur Radio:** Local repeater groups, ham radio forums

### Project Information
- **Version:** 20251124_ESP32 (matches firmware version)
- **License:** Open source for educational and amateur radio use
- **Author:** Community-driven development
- **Latest Updates:** Check GitHub repository for current version

---

**73 and enjoy your ESP32 MMDVM Hotspot!** 📻✨

*"Bringing modern digital amateur radio to the maker community."*
