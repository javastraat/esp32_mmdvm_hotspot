# ESP32 MMDVM Hotspot

A professional ESP32-based DMR hotspot with MMDVM modem support, real-time web interface, and BrandMeister network integration. **Network → RF transmission confirmed working!**

## 🎯 Project Status

**Current Release:** Beta - December 12, 2025  
**Firmware Version:** 20251212_ESP32_BETA

### Confirmed Working
- **Network → RF Transmission** - Receive DMR from BrandMeister and transmit over RF (user verified!)
- **Full DMR Protocol** - BrandMeister network integration with SHA256 authentication
- **MMDVM Communication** - Complete protocol implementation (115200 baud, GPIO 43/44/13)
- **Real-time User Lookup** - RadioID.net API integration with callsign/name/location
- **DMR Activity Display** - Live transmission monitoring with dual-slot support
- **Transmission History** - Last 15 DMR transmissions with duration tracking
- **Professional Web Interface** - Responsive design with dark/light themes
- **Multi-Network WiFi** - Primary + 5 backup networks with auto-failover
- **OLED Display** - Real-time status on 128x64 SSD1306 (optional)
- **RGB LED Indicators** - Visual TX/RX status feedback (GPIO 40/41/42)
- **Configuration Management** - Import/export, backup/restore
- **OTA Updates** - GitHub download + file upload
- **Debug Controls** - Toggle verbose logging (DEBUG_MMDVM, DEBUG_NETWORK)

### In Development
- **RF → Network Path** - Receive DMR from radio and forward to network (RX path)
- **RSSI Monitoring** - Signal strength reporting from modem

### Planned Features
- **D-Star** - D-Star network and protocol support
- **YSF/Fusion** - System Fusion protocol
- **P25** - Project 25 digital voice
- **NXDN** - NXDN protocol
- **POCSAG** - Paging support

## Hardware Requirements

### Tested & Confirmed Working
- **Board:** LILYGO T-ETH-Elite ESP32-S3 with MMDVM HS Hat
- **MMDVM Firmware:** MMDVM_HS_Hat-v1.6.1 20231115_WPSD (14.7456MHz ADF7021)
  - Firmware by: CA6JAU, G4KLX, W0CHP
  - GitID: #7e16099
- **Baud Rate:** 115200 (confirmed working)
- **Antenna:** UHF/VHF (e.g., 70cm for 434MHz)

### Compatible Hardware
**ESP32 Boards:**
- ESP32-S3 (LILYGO T-ETH-Elite - recommended)
- ESP32-WROOM-32
- ESP32-WROVER
- Any ESP32 with available UART pins

**MMDVM Modems:**
- MMDVM HS Hat (tested - fully working ✅)
- JumboSPOT
- ZUMspot  
- MMDVM_HS (Hotspot)
- Any MMDVM with 14.7456MHz or 12.288MHz crystal

### Optional Components
- **OLED Display:** SSD1306 128x64 I2C (real-time status)
- **RGB LED:** Visual TX/RX indicators (GPIO 40/41/42)

## Pin Configuration

### MMDVM Connection (LILYGO T-ETH-Elite ESP32-S3) Fits modem on RPI Headers
```
ESP32 GPIO    MMDVM Hat     Function
----------    ---------     --------
GPIO 43       → RX          ESP32 TX to MMDVM RX (sends commands/data)
GPIO 44       ← TX          ESP32 RX from MMDVM TX (receives responses)
GPIO 13       → (Wakeup)    Keeps MMDVM active (requires UART activity)
GPIO 12       ← (LED)       Status LED monitoring (read-only)
GPIO 0        → PTT         Push-to-Talk Control
GPIO 38       → COS/LED     Carrier Detect LED
3.3V          → VCC         Power Supply (3.3V only!)
GND           → GND         Ground
```

**Critical Notes:**
- **GPIO 13 Wakeup:** MMDVM requires continuous UART activity on this pin to stay active
- **Baud Rate:** 115200 confirmed working (SERIAL_8N1)
- **Timing:** UART .flush() after writes is critical for reliability
- **Voltage:** 3.3V only - DO NOT use 5V

### OLED Display (Optional)
```
ESP32 GPIO    OLED Display  Function
----------    ------------  --------
GPIO 17       → SDA         I2C Data
GPIO 18       → SCL         I2C Clock
3.3V          → VCC         Power Supply
GND           → GND         Ground
```

### RGB LED (Optional)
```
ESP32 GPIO    LED Color     Function
----------    ---------     --------
GPIO 41       → Red         Status indicator
GPIO 40       → Green       TX indicator
GPIO 42       → Blue        RX indicator
```

## Quick Start

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

**Built-in ESP32 Libraries** (no installation needed):
- WiFi (WiFi network connectivity)
- WebServer (Professional web interface)
- ESPmDNS (Network discovery)
- Preferences (Configuration storage with NVS)
- HTTPClient (OTA firmware downloads from GitHub)
- Update (OTA firmware flashing)
- WiFiClientSecure (Secure HTTPS connections)
- mbedtls (SHA256 cryptographic authentication)
- nvs_flash (NVS partition management for factory reset)

**Additional Libraries** (install via Arduino Library Manager):
- **Adafruit GFX Library** (for OLED display graphics)
- **Adafruit SSD1306** (for OLED display driver)

To install in Arduino IDE:
1. Go to Sketch → Include Library → Manage Libraries
2. Search for "Adafruit GFX" and click Install
3. Search for "Adafruit SSD1306" and click Install

### 3. Configuration

Edit `config.h` with your settings:

```cpp
// Firmware Version
#define FIRMWARE_VERSION "20251212_ESP32_BETA"

// WiFi Configuration
#define WIFI_SSID "YourWiFiNetwork"
#define WIFI_PASSWORD "YourWiFiPassword"

// DMR Credentials (get from BrandMeister)
#define DMR_CALLSIGN "N0CALL"        // Your callsign
#define DMR_ID 1234567               // Your 7-digit DMR ID
#define DMR_PASSWORD "passw0rd"      // Your hotspot password
#define DMR_SERVER "44.131.4.1"      // BrandMeister US server
#define DMR_COLORCODE 1              // Usually 1 for BrandMeister

// Hardware Pins (LILYGO T-ETH-Elite ESP32-S3)
#define MMDVM_RX_PIN 44              // ESP32 RX from MMDVM TX
#define MMDVM_TX_PIN 43              // ESP32 TX to MMDVM RX
#define MMDVM_WAKEUP_PIN 13          // Keeps MMDVM active
#define MMDVM_SERIAL_BAUD 115200     // MMDVM baud rate

// RF Settings
#define MMDVM_FREQUENCY 434000000    // 434 MHz (adjust for your region)
#define MMDVM_RF_LEVEL 100           // RF power 0-100%

// Display (optional)
#define ENABLE_OLED true             // Enable OLED display
#define OLED_I2C_ADDRESS 0x3C        // Usually 0x3C or 0x3D

// Debug Options (set false for clean logs)
#define DEBUG_MMDVM false            // MMDVM TX frame debug
#define DEBUG_NETWORK false          // Network keepalive debug
#define DEBUG_DMR false              // DMR protocol debug

// Mode Defaults (must enable via web interface)
#define DEFAULT_MODE_DMR false       // DMR OFF by default
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
6. **Enable DMR mode** in Mode Configuration (OFF by default)
7. Monitor Serial Monitor for "DMR TX START" to confirm RF transmission

## How It Works

### Network → RF Transmission Path (Working!)
When someone transmits on BrandMeister network:

1. **Network Packet** - ESP32 receives DMRD packet from Brand Meister (UDP port 62031)
2. **User Lookup** - RadioID.net API fetches callsign/name/location (cached for performance)
3. **Activity Display** - Web interface and OLED show live transmission
4. **DMR START Command** - ESP32 sends `CMD_DMR_START (0x1D)` to put modem in TX mode
5. **Frame Transmission** - DMR frames sent to modem via `CMD_DMR_DATA2 (0x1A)` with 55ms delay
6. **RF Output** - MMDVM modem transmits on configured frequency
7. **TX END** - After 500ms timeout, sends `CMD_DMR_START (0x00)` to exit TX mode
8. **History Logging** - Transmission added to history with duration

**Example Log:**
```
DMR: Slot2 Seq=0 3120545->TG91 [START] Type=VOICE_LC_HDR
Station: KN4MZQ (3120545) - Jeffrey A from Walkertown, United States
[MMDVM] DMR TX START - KN4MZQ
[MMDVM] DMR TX STOP
DMR: Slot2 Seq=0-6 3120545->TG91 [END]
Adding to history: KN4MZQ (3120545) -> TG91 Duration: 1s
```

### Critical Timing
- **DMR Frame Timing:** 60ms between frames
- **Buffer Prevention:** 55ms delay after each frame prevents modem buffer overflow
- **TX Timeout:** 500ms without frames triggers automatic TX stop
- **Single START:** Only one DMR_START per transmission (not per frame)

## Web Interface Features

Once connected, access the web interface at the ESP32's IP address. Default login: **admin / pi-star**

### Home Dashboard (`/`)
The main landing page provides comprehensive status overview:

**Quick Status Card:**
- WiFi connection status (Connected/AP Mode/Disconnected) with IP address
- DMR network login status with authentication state
- MMDVM hardware ready status ([OK] or [ERR])

**Network & Activity Card:**
- DMR server address and connection details
- Current talkgroup (if active, otherwise shows "None")
- RX/TX frequencies (displayed in MHz)
- Color code and power settings

**System Information:**
- ESP32 firmware version
- Navigation guide to other web pages
- Overview of available features

### WiFi Configuration (`/wificonfig`)
**Multi-Network Support:**
- **Primary Network** - Main WiFi configured in config.h file
- **5 Backup Networks** - Additional WiFi slots (Home, Mobile, Work, Friends, Other)
- **Automatic Failover** - Tries backup networks if primary fails
- **Network Scanner** - Discover and select nearby WiFi networks
- **Access Point Mode** - Creates hotspot if all WiFi networks fail

**Configuration Import/Export:**
- Export current settings to downloadable JSON file
- Import previously saved configuration files
- Complete backup and restore functionality

### DMR Configuration (`/modeconfig`)  
**BrandMeister Integration:**
- **Global Server List** - Built-in list with 40+ servers worldwide
- **Credential Management** - Callsign, DMR ID, and password configuration
- **Frequency Settings** - RX/TX frequency, power, and color code
- **Location Setup** - GPS coordinates and site information
- **Protocol Mode Selection** - Enable/disable digital modes (DMR functional, others planned)
- **DMR Mode Control** - Toggle DMR network connection (default: OFF for fresh installs)
- **Future Protocol Support** - D-Star, YSF, P25, NXDN, POCSAG (configuration ready, protocols not implemented)

**Supported Regions:**
- Europe: Netherlands, Germany, UK, France, Italy, Spain
- North America: USA, Canada  
- Asia-Pacific: Australia, Japan, Thailand
- Others: South Africa, Brazil, New Zealand

### Serial Monitor (`/serialmonitor`)
**Real-time Logging:**
- Live MMDVM communication display with auto-refresh
- Circular buffer storing 50 most recent log messages
- Network packet monitoring and authentication status
- Debug information for troubleshooting
- Pause/resume monitoring functionality
- Clear logs and configurable refresh rates

### Status Page (`/status`)
**System Metrics:**
- DMR network connection status
- WiFi signal strength and network details
- Memory usage and system uptime
- Hardware status indicators

### Admin Panel (`/admin`)
**System Management:**
- Restart system services or complete reboot
- Complete factory reset (erases all NVS data)
- Configuration export/import with JSON format
- Show stored preferences with password masking for security
- Hostname configuration (mDNS: esp32-mmdvm.local)
- Verbose logging toggle (enable/disable keepalive messages)
- Web interface credentials management (username/password)

**OTA Firmware Updates:**
- **GitHub Download** - Automatic firmware updates from configured URL
- **File Upload** - Manual firmware file upload via web browser
- **Three-Step Process** - Download → Upload → Flash with confirmation
- **Secure Updates** - HTTPS downloads with progress indication
- **Safety Features** - Pre-flash validation and error handling

**Advanced Features:**
- **Show Preferences** - Complete ESP32 NVS storage viewer with 17+ settings
- **Password Security** - All password fields automatically masked with toggle
- **Storage Analytics** - Data type identification and size reporting
- **System Diagnostics** - Real-time heap memory and storage utilization
- **Hostname Management** - Configurable mDNS hostname (esp32-mmdvm.local)
- **Verbose Logging** - Toggle keepalive message visibility in logs
- **Credential Management** - Change web interface username/password

### Dark/Light Theme System
**Professional UI Theming:**
- Toggle between dark and light modes via navigation button
- Theme preference stored in browser localStorage
- Consistent theming across all pages
- Smooth transitions with CSS variables
- Professional appearance without decorative elements

### Web Authentication System
**Security Features:**
- HTTP Basic Authentication on all pages
- Configurable username/password via config.h or web interface
- Default credentials: admin / pi-star
- Password change functionality in admin panel
- Secure credential storage in ESP32 NVS

## DMR Network Support

### Authentication Flow
1. **RPTL** - Initial login with DMR ID
2. **RPTK** - SHA256 authentication with salt+password
3. **RPTC** - Configuration packet with station details
4. **RPTPING** - Keepalive every 5 seconds

### Network Protocols
- Full BrandMeister protocol implementation
- DMRplus compatibility
- Custom server support with manual configuration
- Automatic reconnection on network failures

## Advanced Features

### Multi-Network WiFi Management
```cpp
// Primary WiFi (configured in config.h)
#define WIFI_SSID "YourNetwork"
#define WIFI_PASSWORD "YourPassword"

// WiFi Network Structure
struct WiFiNetwork {
  String label;    // User-friendly name (Home, Mobile, Work, Friends, Other)
  String ssid;     // Network SSID
  String password; // Network password
};

// 5 Backup WiFi Networks (configured via web interface)
WiFiNetwork wifiNetworks[5];
// Stored in ESP32 Preferences as:
// - alt_label1-5, alt_ssid1-5, alt_password1-5
// - Automatic failover in labeled order
// - Web-based configuration with network scanner
```

### Configuration Storage System
**ESP32 NVS Preferences:**
- WiFi networks (primary + 5 backups)
- DMR credentials and server settings
- Web interface credentials
- Theme preferences and system settings
- Automatic corruption detection and repair

### OTA Update System
**Dual Method Support:**
1. **GitHub Integration**
   - Automatic version checking via OTA_VERSION_URL
   - Secure HTTPS download from OTA_UPDATE_URL
   - Progress indication and error handling
   
2. **File Upload**
   - Web browser-based firmware upload
   - Direct .bin file flashing
   - Validation and verification

**Security Features:**
- HTTPS-only downloads for GitHub updates
- File size and format validation
- Rollback capability on failed updates
- Progress monitoring with status feedback

## Hardware Configuration

### GPIO Pin Assignments
```cpp
#define MMDVM_RX_PIN 16        // Serial RX from MMDVM
#define MMDVM_TX_PIN 17        // Serial TX to MMDVM
#define MMDVM_PTT_PIN 4        // Push-to-Talk control
#define MMDVM_COS_LED_PIN 2    // Carrier detect LED
#define STATUS_LED_PIN 2       // System status LED (shared)
```

### Serial Configuration
```cpp
#define MMDVM_SERIAL_BAUD 115200    // MMDVM communication speed
// Uses Serial2 for MMDVM communication
// Serial (USB) for debug output at 115200 baud
```

### LED Status Indicators

#### Standard Status LED (GPIO 2)
- **OFF** - System starting up
- **STEADY** - Connected to WiFi and DMR network
- **FAST_BLINK** - Connecting to WiFi
- **SLOW_BLINK** - Access Point mode active
- **Automatic Control** - LED state managed by setLEDMode() and updateStatusLED() functions

#### RGB LED Status Indicator (Optional)

**Hardware Setup:**
Connect RGB LEDs to the following pins:
- Red LED: GPIO 41
- Green LED: GPIO 40
- Blue LED: GPIO 42

**Configuration (config.h):**
```cpp
// RGB LED Settings
#define ENABLE_RGB_LED true          // Enable/disable RGB LED
#define RGB_LED_BRIGHTNESS 25        // Overall brightness (25 = ~10%)
#define RGB_LED_IDLE_BRIGHTNESS 10   // Idle state brightness (dim)
#define RGB_LED_ACTIVE_BRIGHTNESS 50 // TX/RX brightness (brighter but still dimmed)
```

**Status Colors:**

| Status | Color | Brightness | When |
|--------|-------|------------|------|
| **Disconnected** | Red | Dim (10) | No network connection |
| **AP Mode** | Purple | Dim (10) | Access Point mode active |
| **Connecting** | Blue blinking | Flashing | Connecting to network |
| **Network Connected** | Blue | Very dim (10) | Connected but idle |
| **Transmitting** | Green | Medium (50) | TX - Sending to network |
| **Receiving** | Red | Medium (50) | RX - Receiving from network |

**Features:**
- **Dimmed by default** - All colors use the brightness settings you can adjust in config.h
- **PWM control** - Smooth brightness control using ESP32 hardware PWM
- **Activity feedback** - Flashes green when transmitting, red when receiving
- **Network status** - Shows connection state at a glance
- **Non-blocking** - Uses PWM so it doesn't affect performance

**Files:**
- `RGBLedController.h` - RGB LED controller class
- `config.h` - LED configuration settings
- `esp32_mmdvm_hotspot.ino` - Integrated LED control

**Brightness Adjustment:**
Edit the values in config.h to adjust brightness:
- Lower numbers = dimmer (recommended for bright LEDs)
- Higher numbers = brighter
- Range: 0-255 (0=off, 255=full brightness)

## Network Configuration

### WiFi Failover Logic
1. Try primary WiFi from config.h
2. If failed, try backup networks 1-5 in order
3. If all networks fail, start Access Point mode
4. Retry primary network periodically while in AP mode

### DMR Network Settings
```cpp
// Example BrandMeister configuration
#define DMR_SERVER "44.131.4.1"    // Primary BrandMeister US
#define DMR_PORT 62031              // Standard DMR port
#define LOCAL_PORT 62032            // Local UDP port

// Authentication
#define DMR_CALLSIGN "YourCall"     // Amateur radio callsign
#define DMR_ID 1234567              // 7-digit DMR ID
#define DMR_PASSWORD "yourpass"     // BrandMeister password
```

## Troubleshooting

### Common Issues

#### WiFi Connection Problems
- **Check config.h settings** - Verify SSID/password accuracy
- **Use WiFi scanner** - Access via web interface at 192.168.4.1 in AP mode
- **Signal strength** - Ensure ESP32 is within range of WiFi router
- **Backup networks** - Configure multiple WiFi slots for reliability

#### DMR Authentication Failures
- **Verify credentials** - Check callsign, DMR ID, and password on BrandMeister
- **Server selection** - Try different BrandMeister servers from built-in list
- **Network connectivity** - Ensure internet access for DMR authentication
- **Check serial logs** - Monitor authentication flow via web interface

#### MMDVM Communication Issues
- **Hardware connections** - Verify GPIO pin connections to MMDVM hat
- **Power supply** - Ensure adequate 3.3V power for ESP32 and MMDVM
- **Serial settings** - Confirm 115200 baud rate configuration
- **MMDVM firmware** - Update MMDVM hat firmware if necessary

#### OTA Update Problems
- **Network connectivity** - Ensure stable internet connection
- **GitHub URL** - Verify OTA_UPDATE_URL points to valid .bin file
- **File size** - Check available flash memory for update
- **HTTPS certificates** - Ensure system time is accurate for SSL validation

### Advanced Debugging

#### Serial Monitor Analysis
Monitor ESP32 serial output at 115200 baud for detailed logs:
```
=== ESP32 MMDVM Hotspot ===
Initializing...
WiFi Connected [Primary]!
IP address: 192.168.1.100
DMR Login: RPTL sent
DMR Auth: RPTK sent (SHA256)
DMR Config: RPTC sent
DMR Status: Connected
```

#### Show Preferences Tool
Access `/showprefs` for complete system state:
- All stored WiFi networks (passwords masked)
- DMR configuration settings
- Web interface credentials (passwords masked)
- Theme and system preferences
- NVS storage utilization

#### Factory Reset Procedure
1. Access admin panel at `/admin`
2. Click "Complete Factory Reset" 
3. Confirm erasure of all NVS data
4. System will restart with default settings
5. Reconfigure via Access Point mode

### Performance Optimization

#### Memory Management
- Monitor heap usage via status page
- Clear serial logs periodically
- Restart system services if memory low

#### Network Stability
- Use quality WiFi router with good signal strength
- Configure multiple backup WiFi networks
- Select geographically closest BrandMeister server

#### DMR Performance
- Ensure stable internet connection
- Monitor keepalive status in serial logs
- Verify antenna SWR and connections

## Development Information

### File Structure
```
esp32_mmdvm_hotspot/
├── esp32_mmdvm_hotspot.ino    # Main firmware
├── webpages.h                 # Complete web interface
├── config.h                   # Hardware/network configuration
├── README.md                  # This documentation
├── version.txt                # Current firmware version
├── make-bin.sh               # Build script
└── web/                      # Modular web components (legacy)
    ├── common/
    ├── pages/
    └── handlers/
```

### Key Functions
- `setupWiFi()` - Multi-network WiFi connection with automatic failover
- `handleMMDVMSerial()` - MMDVM protocol processing and packet handling
- `connectToDMRNetwork()` - BrandMeister authentication and connection management
- `handleDownloadUpdate()` - GitHub firmware download with HTTPS validation
- `handleUploadFirmware()` - Web browser file upload with progress tracking
- `handleFlashFirmware()` - Firmware flashing with safety validation
- `handleShowPreferences()` - Advanced NVS storage debugging with password masking
- `handleSaveHostname()` - mDNS hostname configuration management
- `handleSaveVerbose()` - Verbose logging control (keepalive message visibility)
- `handleSaveUsername()` - Web interface username management
- `handleSavePassword()` - Web interface password management
- `handleExportConfig()` - JSON configuration backup generation
- `handleImportConfig()` - JSON configuration restore functionality
- `handleGetLogs()` - Serial log retrieval for web interface display
- `handleClearLogs()` - Clear serial monitor log buffer
- `handleWifiScan()` - WiFi network discovery and RSSI reporting
- `handleSaveModes()` - Digital protocol mode enable/disable management
- `handleReboot()` - System restart functionality
- `handleRestartServices()` - Service restart without full reboot
- `handleSaveConfig()` - WiFi configuration persistence
- `handleSaveDMRConfig()` - DMR settings persistence  
- `handleResetConfig()` - Factory reset confirmation page
- `handleConfirmReset()` - Execute complete NVS erasure
- `setLEDMode()` - Control ESP32 status LED (OFF/STEADY/FAST_BLINK/SLOW_BLINK)
- `updateStatusLED()` - LED state machine for visual status indication
- `sendMMDVMCommand()` - Low-level MMDVM serial communication
- `logSerial()` - Standard logging with web buffer storage
- `logSerialVerbose()` - Conditional verbose logging (keepalive messages)

### Protocol Implementation
- **DMR Protocol** - Complete BrandMeister authentication with SHA256
- **MMDVM Serial** - Full packet processing and command handling (0xE0 frame start)
- **Buffer Management** - 512-byte RX/TX buffers with overflow protection
- **Web Interface** - Professional responsive design with theme system
- **Configuration Management** - ESP32 NVS storage with import/export
- **DMR State Machine** - DISCONNECTED → WAITING_LOGIN → WAITING_AUTH → CONNECTED
- **Serial Log Buffer** - 50-message circular buffer with overflow protection

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

// Protocol Defaults (all OFF by default)
#define DEFAULT_MODE_DMR false      // Must be enabled via web interface
#define DEFAULT_MODE_DSTAR false    // Not yet implemented
#define DEFAULT_MODE_YSF false      // Not yet implemented
#define DEFAULT_MODE_P25 false      // Not yet implemented
#define DEFAULT_MODE_NXDN false     // Not yet implemented
#define DEFAULT_MODE_POCSAG false   // Not yet implemented

// Debug Configuration
#define DEBUG_SERIAL true          // Enable serial debug output
#define DEBUG_MMDVM false          // Enable MMDVM protocol debug
#define DEBUG_NETWORK false        // Enable network debug
#define DEBUG_DMR false            // Enable DMR protocol debug
#define DEBUG_PASSWORD false       // Enable password debug output

// Feature Flags
#define ENABLE_OLED false          // OLED display support (not implemented)
#define ENABLE_WEBSERVER false     // Web interface (always enabled in current build)
#define ENABLE_OTA false           // OTA updates (always enabled in current build)
#define ENABLE_MDNS true           // mDNS hostname resolution

// Network & Protocol Timeouts
#define NETWORK_KEEPALIVE_INTERVAL 5000  // DMR keepalive interval (5 seconds)
#define NETWORK_TIMEOUT 30000            // General network timeout (30 seconds)
#define MMDVM_RESPONSE_TIMEOUT 1000      // MMDVM serial response timeout (1 second)
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

## Advanced Features

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

## Development and Customization

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

## License

This project is open source. Please respect amateur radio regulations and licensing requirements in your jurisdiction.

## Contributing

Contributions welcome! Please maintain professional code standards and update documentation for any new features.

## Support

For support:
1. Check serial monitor output for detailed error messages
2. Use Show Preferences tool for system state debugging
3. Try factory reset for persistent configuration issues
4. Verify hardware connections and power supply
5. Check BrandMeister account status and credentials

---

**Current Version:** 20251128_ESP32  
**Compatible:** ESP32 Arduino Core 2.0.11+  
**License:** Open Source Amateur Radio Project

## BrandMeister Server List

The web interface includes 40+ servers worldwide:

**Europe:** Netherlands (2041), Germany (2621/2622), UK (2341), France (2081/2082), Italy (2222), Spain (2141), Belgium (2061), Austria (2322), Switzerland (2282), Denmark (2382), Sweden (2402), Norway (2421), Finland (2441), Czech Republic (2302), Hungary (2162), Romania (2262), Poland (2602), Portugal (2682), Ireland (2721), Bulgaria (2841), Slovenia (2931), Russia (2502/2503), Ukraine (2551), Greece (2022)

**North America:** USA (3102/3103/3104), Canada (3021), Mexico (3341)

**Asia-Pacific:** Australia (5051), South Korea (4501), China (4602), Malaysia (5021), Philippines (5151)

**Middle East/Africa:** Israel (4251), South Africa (6551)

**South America:** Brazil (7242), Chile (7301)

All servers use standard BrandMeister port 62031.

## Resources and Documentation

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

## Legal and Safety Information

### Amateur Radio License Required
- **Legal Operation:** Valid amateur radio license required in your country
- **Frequency Compliance:** Operate only within authorized amateur bands
- **Power Limits:** Respect maximum RF power regulations
- **Identification:** Proper station identification per local regulations

### Network Ethics  
- **Server Resources:** Don't abuse network servers with excessive connections
- **Authentication:** Keep your hotspot password secure and unique
- **Updates:** Keep firmware updated for security and compatibility
- **Community:** Follow BrandMeister and network-specific guidelines

### RF Safety
- **Antenna Safety:** Use proper antenna with appropriate SWR
- **RF Exposure:** Follow SAR and MPE guidelines for your power level  
- **Grounding:** Ensure proper RF and electrical grounding
- **Environment:** Consider RF exposure to others in your area

## Contributing

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

## Support and Community

### Getting Help
- **Hardware Issues:** MMDVM forums and hardware vendor support
- **DMR Network:** BrandMeister support, network-specific forums
- **ESP32 Development:** ESP32 Arduino community, Espressif forums
- **General Amateur Radio:** Local repeater groups, ham radio forums

### Project Information
- **Version:** 20251128_ESP32 (latest with enhanced status page)
- **License:** Open source for educational and amateur radio use
- **Author:** Community-driven development
- **Latest Updates:** Check GitHub repository for current version

### Recent Updates (20251128_ESP32)
- **Enhanced Status Page:** Comprehensive ESP32 hardware metrics including chip revision, CPU cores, heap statistics, flash speed, SDK version, and formatted uptime display
- **Debug Controls:** Added DEBUG_PASSWORD flag in config.h to control password debug output visibility
- **DMR Mode Default:** Changed default DMR mode to OFF to prevent spam to BrandMeister servers with unconfigured credentials
- **Factory Fresh:** Fresh installations now require explicit DMR mode enable and credential configuration via web interface

---

**73 and enjoy your ESP32 MMDVM Hotspot!**

*"Bringing modern digital amateur radio to the maker community."*
