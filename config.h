/*
 * config.h - Configuration file for ESP32 MMDVM Hotspot
 * 
 * Copy this file and customize for your setup
 */

#ifndef CONFIG_H
#define CONFIG_H

// ===== Firmware Version =====
//#define FIRMWARE_VERSION "20251207_ESP32"  // Update version as needed
#define FIRMWARE_VERSION "20251207_ESP32_BETA"  // Update version as needed

// Define board type if you are using the LilyGo T-Ethernet Elite ESP32-S3 MMDVM board
#define LILYGO_T_ETH_ELITE_ESP32S3_MMDVM

// ===== WiFi Configuration =====
#define WIFI_SSID "TechInc"              // Your WiFi SSID
#define WIFI_PASSWORD "itoldyoualready"  // Your WiFi password

// Fallback Access Point mode settings (when WiFi fails)
#define AP_SSID "ESP32-MMDVM-Config"  // AP SSID
#define AP_PASSWORD "mmdvm1234"       // AP Password

// Default labels for alternate WiFi network slots
#define WIFI_SLOT1_LABEL "Home"     // WiFi Slot 1 Label
#define WIFI_SLOT2_LABEL "Mobile"   // WiFi Slot 2 Label
#define WIFI_SLOT3_LABEL "Work"     // WiFi Slot 3 Label
#define WIFI_SLOT4_LABEL "Friends"  // WiFi Slot 4 Label
#define WIFI_SLOT5_LABEL "Other"    // WiFi Slot 5 Label

// ===== DMR Network Configuration =====
// Choose your network and uncomment the appropriate server

// BrandMeister Servers
#define DMR_SERVER "44.131.4.1"  // BrandMeister US
// #define DMR_SERVER "44.225.62.11"  // BrandMeister EU
// #define DMR_SERVER "45.248.50.1"   // BrandMeister Australia

#define DMR_PORT 62031    // BrandMeister default port
#define LOCAL_PORT 62032  // Local UDP port for hotspot

// Your DMR credentials
#define DMR_CALLSIGN "N0CALL"    // Your callsign
#define DMR_ID 1234567           // Your DMR ID (7 digits)
#define DMR_PASSWORD "passw0rd"  // Your hotspot password from BrandMeister
#define DMR_COLORCODE 1          // Color code (1-15, usually 1 for BrandMeister)

// DMR Hotspot Defaults
#define DMR_LOCATION "ESP32 Hotspot"   // Default location name
#define DMR_DESCRIPTION "ESP32-MMDVM"  // Default description
#define DMR_URL ""                     // Default URL (empty)

// ===== Hardware Pin Configuration =====
// Pin definitions based on board type
#if defined(LILYGO_T_ETH_ELITE_ESP32S3_MMDVM)
// GPIO Pins
#define MMDVM_PTT_PIN 0      // Push-to-talk control
#define MMDVM_COS_LED_PIN 38  // Carrier detect LED
#define STATUS_LED_PIN 38
// Network Pins
#define ETH_MISO_PIN 47
#define ETH_MOSI_PIN 21
#define ETH_SCLK_PIN 48
#define ETH_CS_PIN 45
#define ETH_INT_PIN 14
#define ETH_RST_PIN -1
#define ETH_ADDR 1
// SPI Pins
#define SPI_MISO_PIN 9
#define SPI_MOSI_PIN 11
#define SPI_SCLK_PIN 10
// SD Card Pins
#define SD_MISO_PIN SPI_MISO_PIN
#define SD_MOSI_PIN SPI_MOSI_PIN
#define SD_SCLK_PIN SPI_SCLK_PIN
#define SD_CS_PIN 12

// I2C Pins
#define I2C_SDA_PIN 17
#define I2C_SCL_PIN 18

#else
// GPIO Pins
#define MMDVM_PTT_PIN 0      // Push-to-talk control
#define MMDVM_COS_LED_PIN 2  // Carrier detect LED
#define STATUS_LED_PIN 2     // Status LED (can be same as COS)

#endif

// RGB LED Settings
#define ENABLE_RGB_LED false          // Enable RGB LED status indicator
#define RGB_LED_BRIGHTNESS 25        // LED brightness 0-255 (lower = dimmer, 25 = ~10% brightness)
#define RGB_LED_IDLE_BRIGHTNESS 10   // Idle state brightness 0-255 (very dim when idle)
#define RGB_LED_ACTIVE_BRIGHTNESS 5 // Active (TX/RX) state brightness 0-255

// Ledborg RGB LED Pins (if used)
#define LEDBORG_RED_PIN 41
#define LEDBORG_GREEN_PIN 40
#define LEDBORG_BLUE_PIN 42

// Serial communication
#define MMDVM_SERIAL_BAUD 115200  // MMDVM serial baud rate
#define MMDVM_RX_PIN 44          // MMDVM RX pin
#define MMDVM_TX_PIN 43          // MMDVM TX pin
//40 = green
//41 = red
//42 = blue

// RF Configuration
#define MMDVM_RX_INVERT false   // RX signal inversion
#define MMDVM_TX_INVERT false   // TX signal inversion
#define MMDVM_PTT_INVERT false  // PTT signal inversion
#define MMDVM_TX_DELAY 20       // TX delay in milliseconds (10-50)
#define MMDVM_RX_LEVEL 128      // RX level (0-255, 128 = 50%)
#define MMDVM_TX_LEVEL 128      // TX level (0-255, 128 = 50%)
#define MMDVM_RF_LEVEL 100      // RF output power percentage (0-100)

// Frequency Settings (for software-defined MMDVM hats)
#define MMDVM_FREQUENCY 434000000  // Frequency in Hz (e.g., 434 MHz)
#define MMDVM_TX_FREQ_OFFSET 0     // TX frequency offset in Hz
#define MMDVM_RX_FREQ_OFFSET 0     // RX frequency offset in Hz

// ===== Hotspot Information =====
#define HOTSPOT_CALLSIGN DMR_CALLSIGN
#define HOTSPOT_SUFFIX "HS"      // Suffix for hotspot callsign
#define HOTSPOT_LATITUDE 0.0     // Your latitude (decimal degrees)
#define HOTSPOT_LONGITUDE 0.0    // Your longitude (decimal degrees)
#define HOTSPOT_HEIGHT 0         // Antenna height in meters
#define HOTSPOT_LOCATION "Home"  // Location description
#define HOTSPOT_DESCRIPTION "ESP32 MMDVM Hotspot"

// ===== Network Settings =====
#define NETWORK_KEEPALIVE_INTERVAL 5000  // Keepalive interval in milliseconds
#define NETWORK_TIMEOUT 30000            // Network timeout in milliseconds
#define NETWORK_RECONNECT_DELAY 5000     // Delay before reconnecting

// ===== NTP Time Settings =====
#define NTP_SERVER1 "pool.ntp.org"    // Primary NTP server
#define NTP_SERVER2 "time.nist.gov"   // Secondary NTP server
#define NTP_TIMEZONE_OFFSET 3600         // Timezone offset in seconds (0 = UTC/GMT)
                                      // Examples:
                                      //   UTC: 0
                                      //   EST: -5 * 3600 = -18000
                                      //   PST: -8 * 3600 = -28800
                                      //   CET: 1 * 3600 = 3600
                                      //   AEST: 10 * 3600 = 36000
#define NTP_DAYLIGHT_OFFSET 0         // Daylight saving offset in seconds (0 = no DST, 3600 = 1 hour DST)

// ===== DMR User Database API Settings =====
#define DMR_API_URL "https://radioid.net/api/dmr/user/?id="  // RadioID.net API endpoint
// Alternative APIs:
// #define DMR_API_URL "https://database.radioid.net/api/dmr/user/?id="  // Alternative RadioID mirror
// #define DMR_API_URL "https://ham-digital.org/api/dmr/user/?id="       // Ham-Digital.org API
#define DMR_API_TIMEOUT 3000              // API request timeout in milliseconds

// ===== DMR Activity & History Settings =====
#define DMR_HISTORY_SIZE 15               // Number of recent transmissions to display (shown on home page)
#define DMR_ACTIVITY_TIMEOUT 3000         // Timeout for active transmission display in milliseconds
#define QRZ_LOOKUP_URL "https://www.qrz.com/db/"  // QRZ.com callsign lookup URL

#if defined(LILYGO_T_ETH_ELITE_ESP32S3_MMDVM) // More memory available
#define DMR_USER_CACHE_SIZE 500            // Number of DMR user info lookups to cache
#define DMR_CALLSIGN_CACHE_SIZE 500        // Number of callsign lookups to cache
#else
#define DMR_USER_CACHE_SIZE 50            // Number of DMR user info lookups to cache
#define DMR_CALLSIGN_CACHE_SIZE 50        // Number of callsign lookups to cache
#endif

// ===== Debug Settings =====
#define DEBUG_SERIAL true     // Enable serial debug output
#define DEBUG_MMDVM false     // Enable MMDVM protocol debug
#define DEBUG_NETWORK false   // Enable network debug
#define DEBUG_DMR false       // Enable DMR protocol debug
#define DEBUG_PASSWORD false  // Enable password debug output (shows password length/last2 chars)

// ===== mDNS Settings =====
#define ENABLE_MDNS true             // Enable mDNS (find hotspot by hostname)
#define MDNS_HOSTNAME "esp32-mmdvm"  // mDNS hostname

// ===== Web Interface Settings =====
#define ENABLE_WEBSERVER true                            // Enable web configuration interface
#define WEB_USERNAME "admin"                             // Web interface username
#define WEB_PASSWORD "pi-star"                           // Default web interface password
#define COPYRIGHT_TEXT "&copy; 2025 by PD2EMC & PD8JO"  // Footer copyright text

// Footer Links Configuration
#define FOOTER_LINK1_URL "https://einstein.amsterdam"                              // First footer link URL
#define FOOTER_LINK1_TEXT "einstein.amsterdam"                                     // First footer link text
#define FOOTER_LINK2_URL "https://pd8jo.nl"                                        // Second footer link URL  
#define FOOTER_LINK2_TEXT "pd8jo.nl"                                               // Second footer link text
#define FOOTER_LINK3_URL "https://github.com/javastraat/esp32_mmdvm_hotspot"      // Third footer link URL
#define FOOTER_LINK3_TEXT "GitHub Project"                                         // Third footer link text

// ===== OTA Update Configuration =====
#define ENABLE_OTA false  // always on for now Enable over-the-air updates
#define OTA_UPDATE_FACTORY_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/firmware/factory-setup.bin"
#define OTA_UPDATE_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/firmware/update.bin"
#define OTA_UPDATE_BETA_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/firmware/update_beta.bin"
#define OTA_VERSION_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/version.txt"
#define OTA_VERSION_BETA_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/version_beta.txt"
#define OTA_TIMEOUT 30000  // OTA download timeout in milliseconds

// OLED Display Settings
#define ENABLE_OLED false       // Enable OLED display support
#define OLED_I2C_ADDRESS 0x3C  // I2C address for OLED display (0x3C or 0x3D)
#define OLED_WIDTH 128         // OLED display width
#define OLED_HEIGHT 64         // OLED display height

// ===== Advanced Settings =====
// Buffer sizes
#define SERIAL_RX_BUFFER_SIZE 512  // MMDVM serial RX buffer size
#define UDP_BUFFER_SIZE 512        // UDP buffer size

// Timeouts
#define MMDVM_RESPONSE_TIMEOUT 1000  // Milliseconds to wait for MMDVM response
#define WIFI_CONNECT_TIMEOUT 30000   // Milliseconds to wait for WiFi connection

// ===== Multi-Protocol Support =====
// Enable/disable protocols (requires appropriate MMDVM firmware)
// These are default values - can be changed via web interface
#define DEFAULT_MODE_DMR false     // DMR mode (functional) - OFF by default, enable via web interface
#define DEFAULT_MODE_DSTAR false   // D-Star mode (not yet implemented)
#define DEFAULT_MODE_YSF false     // YSF/Fusion mode (not yet implemented)
#define DEFAULT_MODE_P25 false     // P25 mode (not yet implemented)
#define DEFAULT_MODE_NXDN false    // NXDN mode (not yet implemented)
#define DEFAULT_MODE_POCSAG false  // POCSAG paging mode (not yet implemented)

// ===== Modem Hardware Type =====
// Default modem type - can be changed via web interface
#define DEFAULT_MODEM_TYPE "mmdvmhshat"  // Default: MMDVM_HS_Hat (DB9MAT & DF2ET)

// ===== Validation =====
#if !defined(DMR_CALLSIGN) || !defined(DMR_ID)
#error "Please configure your DMR callsign and ID in config.h"
#endif

#if DMR_ID < 1000000 || DMR_ID > 9999999
#error "DMR ID must be 7 digits"
#endif

#endif  // CONFIG_H
