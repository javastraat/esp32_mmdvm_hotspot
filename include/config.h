/*
 * ESP32 RTOS MMDVM Configuration
 * Place all configuration parameters here
 */

#ifndef CONFIG_H
#define CONFIG_H

// Firmware version
//#define FIRMWARE_VERSION "20260221_ESP32"
#define FIRMWARE_VERSION "20260301_ESP32_BETA"

// ========================================
// Default User Settings
// ========================================
// Default callsign
#define DMR_CALLSIGN "N0CALL"
// Default DMR ID (not used in this demo, but can be implemented in the future)
#define DMR_ID 1234567
// DMR SSID for owner callsign in owner.txt
#define DMR_SSID 0

// ========================================
// NETWORK CONFIGURATION
// ========================================

// ===== WiFi Configuration =====
#define WIFI_SLOT_LABEL "Default"       // WiFi Slot Label
#define WIFI_SSID "TechInc"             // Your WiFi SSID
#define WIFI_PASSWORD "itoldyoualready" // Your WiFi password

#define WIFI_SLOT1_LABEL "Home" // WiFi Slot 1 Label
#define WIFI_SSID1 ""           // Your WiFi SSID
#define WIFI_PASSWORD1 ""       // Your WiFi password

#define WIFI_SLOT2_LABEL "Mobile" // WiFi Slot 2 Label
#define WIFI_SSID2 ""             // Your WiFi SSID
#define WIFI_PASSWORD2 ""         // Your WiFi password

#define WIFI_SLOT3_LABEL "Work" // WiFi Slot 3 Label
#define WIFI_SSID3 ""           // Your WiFi SSID
#define WIFI_PASSWORD3 ""       // Your WiFi password

#define WIFI_SLOT4_LABEL "Friends" // WiFi Slot 4 Label
#define WIFI_SSID4 ""              // Your WiFi SSID
#define WIFI_PASSWORD4 ""          // Your WiFi password

#define WIFI_SLOT5_LABEL "Other" // WiFi Slot 5 Label
#define WIFI_SSID5 ""            // Your WiFi SSID
#define WIFI_PASSWORD5 ""        // Your WiFi password

// WiFi Soft AP fallback - Starts if no WiFi/Ethernet after retries
#define WIFI_AP_SSID "MMDVM-Setup"
#define WIFI_AP_PASSWORD "mmdvm1234" // Min 8 characters
#define WIFI_AP_CHANNEL 1
#define WIFI_MAX_RETRIES 5 // Start AP after this many failed attempts

// Ethernet Configuration
#define ETH_ENABLED false // Set to true to enable Ethernet (can run alongside WiFi)
#define ETH_DEBUG false   // Set to true to enable Ethernet debug messages

// Fallback DNS Server (added after DHCP, used if primary DNS fails)
#define DNS_FALLBACK_ENABLED false
#define DNS_FALLBACK_IP "8.8.8.8" // Google DNS (or use "1.1.1.1" for Cloudflare)

// mDNS Settings
#define ENABLE_MDNS false           // Enable mDNS (find hotspot by hostname)
#define MDNS_HOSTNAME "esp32-mmdvm" // mDNS hostname

// NTP Time Synchronization Configuration
#define NTP_ENABLED false
#define NTP_SERVER "ntp.time.nl"     // NTP server to sync with (can be pool.ntp.org or your local NTP server)
#define NTP_GMT_OFFSET_SEC 3600      // GMT+1 (1 hour = 3600 seconds)
#define NTP_DAYLIGHT_OFFSET_SEC 0    // Daylight saving time offset in seconds
#define NTP_SYNC_INTERVAL_MS 3600000 // Sync every hour (3600000 ms)

// MQTT Configuration
#define MQTT_ENABLED false
#define MQTT_BROKER "10.0.0.1" // MQTT broker IP or hostname
#define MQTT_PORT 1883             // MQTT broker port (default: 1883)
#define MQTT_USER ""               // MQTT username (leave empty if no auth)
#define MQTT_PASSWORD ""           // MQTT password (leave empty if no auth)
// MQTT Client ID (will be generated using mDNS hostname for uniqueness)
//
// MQTT Topics
#define MQTT_STATUS_TOPIC "mmdvm/status"     // Topic for status messages
#define MQTT_LOGS_TOPIC "mmdvm/logs"         // Topic for log messages
#define MQTT_HARDWARE_TOPIC "mmdvm/hardware" // Topic for hardware info messages
#define MQTT_SUBSCRIBE_TOPIC "mmdvm/command" // Topic to subscribe to (leave empty to not subscribe)

// System/Hardware Task Topics
#define MQTT_LOGGER_TASK_TOPIC "mmdvm/task/logger"
#define MQTT_OLED_TASK_TOPIC "mmdvm/task/oled"
#define MQTT_LED_TASK_TOPIC "mmdvm/task/led"
#define MQTT_WIFI_TASK_TOPIC "mmdvm/task/wifi"
#define MQTT_ETH_TASK_TOPIC "mmdvm/task/eth"
#define MQTT_ARDUINO_OTA_TASK_TOPIC "mmdvm/task/arduino_ota"
#define MQTT_NTP_TASK_TOPIC "mmdvm/task/ntp"
#define MQTT_MQTT_CLIENT_TASK_TOPIC "mmdvm/task/mqtt_client"
#define MQTT_WEB_SERVER_TASK_TOPIC "mmdvm/task/web_server"
#define MQTT_SD_CARD_TASK_TOPIC "mmdvm/task/sd_card"
#define MQTT_SENSOR_TASK_TOPIC "mmdvm/task/sensor" // For optional sensor task

// MMDVM/Protocol Task Topics
#define MQTT_MODEM_TASK_TOPIC "mmdvm/task/modem"
#define MQTT_DMR_TASK_TOPIC "mmdvm/task/dmr"
#define MQTT_DSTAR_TASK_TOPIC "mmdvm/task/dstar"
#define MQTT_YSF_TASK_TOPIC "mmdvm/task/ysf"
#define MQTT_P25_TASK_TOPIC "mmdvm/task/p25"
#define MQTT_NXDN_TASK_TOPIC "mmdvm/task/nxdn"
#define MQTT_POCSAG_TASK_TOPIC "mmdvm/task/pocsag"
#define MQTT_DAPNET_TASK_TOPIC "mmdvm/task/dapnet"
// WireGuard MQTT Task Topic
#define MQTT_WG_TASK_TOPIC "mmdvm/task/wireguard"

// MQTT Settings
#define MQTT_SEND_HARDWARE_INFO 30  // Seconds between sending hardware info messages
#define MQTT_HARDWARE_INFO_LOG true // When true, exclude hardware info from serial log (reduce spam)

// WireGuard VPN Configuration
#define WG_ENABLED false
#define WG_LOCAL_IP "10.0.0.2"       // WireGuard interface IP address
#define WG_PRIVATE_KEY ""            // Device WireGuard private key (base64)
#define WG_PUBLIC_KEY ""             // Peer WireGuard public key (base64)
#define WG_ENDPOINT ""               // Server endpoint (IP or hostname)
#define WG_ENDPOINT_PORT 51820       // Server endpoint port
#define WG_DNS "10.0.0.1"           // DNS server for WireGuard
#define WG_ALLOWED_IPS "0.0.0.0/0"

// Web server port
#define WEB_ENABLED true
const int WEB_SERVER_PORT = 80;
#define WEB_USERNAME "admin"                             // Web interface username
#define WEB_PASSWORD "pi-star"                           // Default web interface password

// CW ID Configuration (for regulatory station identification)
#define CWID_ENABLED        false
#define CWID_INTERVAL_MIN   10             // Transmit every 10 minutes

// ========================================
// DMR RELEVANT CONFIGURATION

// ===== DMR Configuration =====
// BrandMeister Servers
//#define DMR_SERVER "44.131.4.1"  // BrandMeister US
//#define DMR_SERVER "44.137.42.20"  // BrandMeister 2041
// #define DMR_SERVER "45.248.50.1"   // BrandMeister Australia
#define DMR_SERVER "api.brandmeister.network"  // Main BrandMeister hub
#define DMR_PORT 62031    // BrandMeister default port
#define DMR_LOCAL_PORT 0  // Local UDP port for hotspot (0 = ephemeral port for NAT compatibility)
// Your DMR credentials
#define DMR_PASSWORD "passw0rd"  // Your hotspot password from BrandMeister
//
#define DMR_HISTORY_SIZE 15               // Number of recent transmissions to display (shown on home page)
#define DMR_ACTIVITY_TIMEOUT 3000         // Timeout for active transmission display in milliseconds
#define DMR_USER_CACHE_SIZE 100            // Number of DMR user info lookups to cache
#define DMR_CALLSIGN_CACHE_SIZE 100        // Number of callsign lookups to cache
#define DMR_API_TIMEOUT 3000              // API request timeout in milliseconds
// #define DEBUG_DMR_LOOKUP                // Uncomment to log SQLite query strings (verbose)
// DMR Network Timing
#define DMR_LOGIN_TIMEOUT 10000          // Login timeout in milliseconds
#define DMR_LOGIN_MAX_RETRIES 3          // Max login retry attempts
#define DMR_KEEPALIVE_INTERVAL 5000      // Keepalive interval in milliseconds
// ===== Hotspot Information =====
#define HOTSPOT_CALLSIGN DMR_CALLSIGN
#define HOTSPOT_SUFFIX "HS"      // Suffix for hotspot callsign
#define HOTSPOT_LATITUDE "0.0"     // Your latitude (decimal degrees)
#define HOTSPOT_LONGITUDE "0.0"    // Your longitude (decimal degrees)
#define HOTSPOT_HEIGHT 0         // Antenna height in meters
#define HOTSPOT_LOCATION "Home"  // Location description
#define HOTSPOT_DESCRIPTION "ESP32 MMDVM Hotspot"
#define HOTSPOT_URL ""                     // Default URL (empty)

// ===== DMR User Database API Settings =====
#define DMR_API_URL "https://radioid.net/api/dmr/user/?id="  // RadioID.net API endpoint
#define QRZ_LOOKUP_URL "https://www.qrz.com/db/"  // QRZ.com callsign lookup URL
// Alternative APIs:
// #define DMR_API_URL "https://database.radioid.net/api/dmr/user/?id="  // Alternative RadioID mirror
// #define DMR_API_URL "https://ham-digital.org/api/dmr/user/?id="       // Ham-Digital.org API
// ========================================
//
// ========================================
// HARDWARE CONFIGURATION
// ========================================

// Built-in LED pin
const int LED_PIN = 38;

// Button pin for OLED toggle
const int BUTTON_PIN = 0;

// OLED Configuration
#define OLED_ENABLED false
const int I2C_SDA_PIN = 17;
const int I2C_SCL_PIN = 18;
const int OLED_I2C_ADDRESS = 0x3C; // I2C address for OLED display (0x3C or 0x3D)
const int OLED_WIDTH = 128;        // OLED display width
const int OLED_HEIGHT = 64;        // OLED display height

// SD Card Configuration
#define SDCARD_ENABLED false
#define SPI_MISO_PIN 9
#define SPI_MOSI_PIN 11
#define SPI_SCLK_PIN 10
#define SD_MISO_PIN SPI_MISO_PIN
#define SD_MOSI_PIN SPI_MOSI_PIN
#define SD_SCLK_PIN SPI_SCLK_PIN
#define SD_CS_PIN 12

// Ethernet SPI Pins (W5500 module)
const int ETH_MISO_PIN = 47;
const int ETH_MOSI_PIN = 21;
const int ETH_SCLK_PIN = 48;
const int ETH_CS_PIN = 45;
const int ETH_INT_PIN = 14;
const int ETH_RST_PIN = -1;
const int ETH_ADDR = 1;
const int ETH_CONNECT_TIMEOUT = 90000; // 90 seconds timeout for Ethernet connection

// ========================================
// MMDVM MODEM HARDWARE CONFIGURATION
// ========================================

// MMDVM Serial Communication (Serial2)
#define MMDVM_SERIAL_BAUD 115200
const int MMDVM_RX_PIN = 44;    // ESP32 RX from MMDVM TX
const int MMDVM_TX_PIN = 43;    // ESP32 TX to MMDVM RX
const int MMDVM_BOOT_PIN = 4;   // BOOT0 pin for bootloader control
const int MMDVM_RESET_PIN = 13; // RESET pin for modem reset

// MMDVM Modem Firmware URLs
#define MMDVM_FIRMWARE_SINGLE_V161_URL "https://github.com/javastraat/esp32_mmdvm_hotspot/raw/refs/heads/main/firmware/mmdvm/mmdvm_hs_hat_fw.bin"
#define MMDVM_FIRMWARE_DUAL_V161_URL "https://github.com/javastraat/esp32_mmdvm_hotspot/raw/refs/heads/main/firmware/mmdvm/mmdvm_hs_dual_hat_fw.bin"
#define MMDVM_FIRMWARE_SINGLE_V152_URL "https://github.com/javastraat/esp32_mmdvm_hotspot/raw/refs/heads/main/firmware/mmdvm/generic_gpio_fw152.bin"

// MMDVM Modem Configuration
const int MMDVM_WAKEUP_PIN = 13; // GPIO 13 keeps modem awake (continuous UART activity required)
const int MMDVM_TX_DELAY = 20;    // TX delay (0-50)
const int MMDVM_RX_LEVEL = 128;   // RX level (0-255)
const int MMDVM_TX_LEVEL = 128;   // TX level (0-255)

// RF Configuration
#define MMDVM_RX_INVERT false   // RX signal inversion
#define MMDVM_TX_INVERT false   // TX signal inversion
#define MMDVM_PTT_INVERT false  // PTT signal inversion
#define MMDVM_RF_LEVEL 100      // RF output power percentage (0-100)


// Frequency Settings (for software-defined MMDVM hats)
#define MMDVM_FREQUENCY 434000000  // Frequency in Hz (e.g., 434 MHz)
#define MMDVM_TX_FREQ_OFFSET 0     // TX frequency offset in Hz
#define MMDVM_RX_FREQ_OFFSET 0     // RX frequency offset in Hz

// DMR Network Settings (can be overridden via web interface or NVS)
#define DMR_RX_FREQ 434000000 // RX Frequency in Hz (434 MHz)
#define DMR_TX_FREQ 434000000 // TX Frequency in Hz (434 MHz)
#define DMR_COLOR_CODE 1      // DMR Color Code (0-15)
#define DMR_RF_POWER 100      // RF power level (0-255, typically 100)

// POCSAG Settings
#define POCSAG_FREQUENCY 439987500 // POCSAG Frequency in Hz (434 MHz)
#define POCSAG_INVERT_POLARITY false // Set true to invert POCSAG frame polarity

// POCSAG API Rate Limiting
#define POCSAG_API_RATE_LIMIT     10     // max requests per window
#define POCSAG_API_RATE_WINDOW_MS 60000  // 60-second window

// DAPNET Network Settings
#define DAPNET_ENABLED      false
#define DAPNET_SERVER       "dapnet.afu.rwth-aachen.de"
#define DAPNET_PORT         43434
#define DAPNET_NODE_CS      ""   // defaults to userCallsign at runtime
#define DAPNET_AUTH_KEY     ""
#define POCSAG_WHITELIST    ""
#define POCSAG_BLACKLIST    ""

// ========================================
// SD CARD DATABASE CONFIGURATION
// ========================================

// SD Card Database Settings
#define SDCARD_DATABASE_DIR "/database"
#define SDCARD_CSV_FILE "/database/radioid.csv"
#define SDCARD_SQLITE_FILE "/database/esp32_database.db"

// Database Download URLs
// Remote URLs for DMR database files
#define SDCARD_CSV_URL "https://raw.githubusercontent.com/DMR-Database/dmr-database-appdata/refs/heads/main/radioid.csv"
#define SDCARD_SQLITE_URL "https://raw.githubusercontent.com/DMR-Database/dmr-database-appdata/refs/heads/main/esp32_database.db"

// Local server for testing (uncomment to use):
// #define SDCARD_CSV_URL "http://192.168.2.173/dmr-database/radioid.csv"
// #define SDCARD_SQLITE_URL "http://192.168.2.173/dmr-database/esp32_database.db"

// ========================================
// WEB INTERFACE CONFIGURATION
// ========================================

// Footer Links Configuration
#define FOOTER_LINK1_TEXT "GitHub Project"
#define FOOTER_LINK1_URL "https://github.com/javastraat/esp32_mmdvm_hotspot"
#define FOOTER_LINK2_TEXT "DMR Database"
#define FOOTER_LINK2_URL "https://dmr-database.github.io"
#define FOOTER_LINK3_TEXT "einstein.amsterdam"
#define FOOTER_LINK3_URL "https://einstein.amsterdam"
#define FOOTER_LINK4_TEXT "pd8jo.nl"
#define FOOTER_LINK4_URL "https://pd8jo.nl"
// Copyright text
#define COPYRIGHT_TEXT "&copy; 2026 by PD2EMC & PD8JO" // Footer copyright text

// ========================================
// FIRMWARE OTA CONFIGURATION
// ========================================

// ArduinoOTA - Upload from Arduino IDE over network
#define ARDUINO_OTA_ENABLED false
#define ARDUINO_OTA_PASSWORD "mmdvm" // Leave empty for no password, or set like "admin"
#define ARDUINO_OTA_PORT 3232        // Default ArduinoOTA port

// OTA Update URLs
#define OTA_VERSION_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/version.txt"
#define OTA_VERSION_BETA_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/version_beta.txt"
#define OTA_UPDATE_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/firmware/update.bin"
#define OTA_UPDATE_BETA_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/firmware/update_beta.bin"

// #define OTA_VERSION_URL "http://192.168.2.220:3000/einstein/esp32-rtos-mmdvm/raw/branch/main/version.txt"
// #define OTA_VERSION_BETA_URL "http://192.168.2.220:3000/einstein/esp32-rtos-mmdvm/raw/branch/main/version_beta.txt"
// #define OTA_UPDATE_URL "http://192.168.2.220:3000/einstein/esp32-rtos-mmdvm/raw/branch/main/firmware/update.bin"
// #define OTA_UPDATE_BETA_URL "http://192.168.2.220:3000/einstein/esp32-rtos-mmdvm/raw/branch/main/firmware/update_beta.bin"


#define OTA_UPDATE_FACTORY_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/firmware/factory-setup.bin"
#define OTA_FIRMWARE_BASE_URL "https://github.com/javastraat/esp32_mmdvm_hotspot/releases/download/"
#define OTA_FIRMWARE_FACTORY_URL "https://github.com/javastraat/esp32_mmdvm_hotspot/raw/refs/heads/main/firmware/factory/factory-setup.ino.bin"

// #define OTA_FIRMWARE_RTOS_URL "http://192.168.2.220:3000/einstein/esp32-rtos-mmdvm/raw/branch/main/firmware/update.bin"
// #define OTA_FIRMWARE_RTOS_URL_BETA "http://192.168.2.220:3000/einstein/esp32-rtos-mmdvm/raw/branch/main/firmware/update_beta.bin"
#define OTA_FIRMWARE_RTOS_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/firmware/update.bin"
#define OTA_FIRMWARE_RTOS_URL_BETA "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/firmware/update_beta.bin"



#define OTA_TIMEOUT 30000 // 30 seconds timeout for OTA downloads

// Logger Configuration
#define LOG_BUFFER_SIZE 50      // Number of log lines to keep
#define LOG_LINE_MAX_LENGTH 128 // Max characters per line

// ========================================
// MMDVM PROTOCOL CONFIGURATION
// ========================================

// Multi-Protocol Support
// Enable/disable protocols (requires appropriate MMDVM firmware)
// These are default values - can be changed via web interface
#define DEFAULT_MODE_DMR false    // DMR mode (functional) - OFF by default, enable via web interface
#define DEFAULT_MODE_DSTAR false  // D-Star mode (not yet implemented)
#define DEFAULT_MODE_YSF false    // YSF/Fusion mode (not yet implemented)
#define DEFAULT_MODE_P25 false    // P25 mode (not yet implemented)
#define DEFAULT_MODE_NXDN false   // NXDN mode (not yet implemented)
#define DEFAULT_MODE_POCSAG false // POCSAG paging mode (not yet implemented)

// ========================================
// RTOS TASK CONFIGURATION
// ========================================

// Task priorities (0-24, higher = more priority)
const int WIFI_TASK_PRIORITY = 2;
const int ETH_TASK_PRIORITY = 2;
const int WEBSERVER_TASK_PRIORITY = 2;
const int SDCARD_TASK_PRIORITY = 2;
const int OLED_TASK_PRIORITY = 1;
const int LED_TASK_PRIORITY = 1;
const int SENSOR_TASK_PRIORITY = 1;
const int NTP_TASK_PRIORITY = 1;
const int MQTT_TASK_PRIORITY = 1;
const int ARDUINO_OTA_TASK_PRIORITY = 1;
const int MODEM_TASK_PRIORITY = 5; // Very high priority - modem must be responsive
// WireGuard Task Settings
const int WG_TASK_PRIORITY = 1;

// MMDVM Protocol task priorities
const int MMDVM_DMR_PRIORITY = 3; // High priority - real-time protocol
const int MMDVM_DSTAR_PRIORITY = 3;
const int MMDVM_YSF_PRIORITY = 3;
const int MMDVM_P25_PRIORITY = 3;
const int MMDVM_NXDN_PRIORITY = 3;
const int MMDVM_POCSAG_PRIORITY = 2; // Lower - paging is less time-critical

// Task stack sizes (words - 1 word = 4 bytes on ESP32)
// e.g., 10000 words = 40KB actual stack
const int WIFI_TASK_STACK = 10000;
const int ETH_TASK_STACK = 10000;
const int WEBSERVER_TASK_STACK = 10000;
const int SDCARD_TASK_STACK = 8000;
const int OLED_TASK_STACK = 6000; // Increased for I2C operations
const int LED_TASK_STACK = 2000;
const int SENSOR_TASK_STACK = 4000;
const int NTP_TASK_STACK = 4096;
const int MQTT_TASK_STACK = 4096;
const int ARDUINO_OTA_TASK_STACK = 4096;
const int MODEM_TASK_STACK = 12000; // Modem initialization needs more stack
// WireGuard Task Settings
const int WG_TASK_STACK = 8192;

// MMDVM Protocol task stack sizes (words)
const int MMDVM_DMR_STACK = 12000;
const int MMDVM_DSTAR_STACK = 8000;
const int MMDVM_YSF_STACK = 8000;
const int MMDVM_P25_STACK = 8000;
const int MMDVM_NXDN_STACK = 8000;
const int MMDVM_POCSAG_STACK = 6000;
const int MMDVM_DAPNET_PRIORITY = 1; // Low - network IO, not time-critical
const int MMDVM_DAPNET_STACK = 6144; // Extra headroom for mbedTLS HMAC-MD5

// Task timing intervals (milliseconds)
const int WIFI_CONNECT_TIMEOUT = 15000; // WiFi connection timeout (30 seconds)
const int WIFI_CHECK_INTERVAL = 5000;
const int ETH_CHECK_INTERVAL = 5000;
const int SDCARD_CHECK_INTERVAL = 60000; // Check SD card status every 60 seconds
const int OLED_UPDATE_INTERVAL = 1000;
const int LED_ON_TIME = 200;
const int LED_OFF_TIME = 800;
const int SENSOR_READ_INTERVAL = 2000;
const int MQTT_RECONNECT_INTERVAL = 5000; // MQTT reconnect attempt interval

#endif // CONFIG_H
