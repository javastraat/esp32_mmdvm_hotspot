/*
 * config.h - Configuration file for ESP32 MMDVM Hotspot
 * 
 * Copy this file and customize for your setup
 */

#ifndef CONFIG_H
#define CONFIG_H

// ===== WiFi Configuration =====
#define WIFI_SSID "TechInc"
#define WIFI_PASSWORD "itoldyoualready"

// ===== DMR Network Configuration =====
// Choose your network and uncomment the appropriate server

// BrandMeister Servers
#define DMR_SERVER "44.131.4.1"    // BrandMeister US
// #define DMR_SERVER "44.225.62.11"  // BrandMeister EU
// #define DMR_SERVER "45.248.50.1"   // BrandMeister Australia

#define DMR_PORT 62031
#define LOCAL_PORT 62032

// Your DMR credentials
#define DMR_CALLSIGN "N0CALL"      // Your callsign
#define DMR_ID 1234567             // Your DMR ID (7 digits)
#define DMR_PASSWORD "passw0rd"    // Your hotspot password from BrandMeister
#define DMR_COLORCODE 1            // Color code (1-15, usually 1 for BrandMeister)

// ===== MMDVM Hardware Configuration =====
// Serial communication
#define MMDVM_SERIAL_BAUD 115200
#define MMDVM_RX_PIN 16
#define MMDVM_TX_PIN 17

// GPIO Pins
#define MMDVM_PTT_PIN 4            // Push-to-talk control
#define MMDVM_COS_LED_PIN 2        // Carrier detect LED
#define STATUS_LED_PIN 2           // Status LED (can be same as COS)

// RF Configuration
#define MMDVM_RX_INVERT false      // RX signal inversion
#define MMDVM_TX_INVERT false      // TX signal inversion  
#define MMDVM_PTT_INVERT false     // PTT signal inversion
#define MMDVM_TX_DELAY 20          // TX delay in milliseconds (10-50)
#define MMDVM_RX_LEVEL 128         // RX level (0-255, 128 = 50%)
#define MMDVM_TX_LEVEL 128         // TX level (0-255, 128 = 50%)
#define MMDVM_RF_LEVEL 100         // RF output power percentage (0-100)

// Frequency Settings (for software-defined MMDVM hats)
#define MMDVM_FREQUENCY 434000000  // Frequency in Hz (e.g., 434 MHz)
#define MMDVM_TX_FREQ_OFFSET 0     // TX frequency offset in Hz
#define MMDVM_RX_FREQ_OFFSET 0     // RX frequency offset in Hz

// ===== Hotspot Information =====
#define HOTSPOT_CALLSIGN DMR_CALLSIGN
#define HOTSPOT_SUFFIX "HS"        // Suffix for hotspot callsign
#define HOTSPOT_LATITUDE 0.0       // Your latitude (decimal degrees)
#define HOTSPOT_LONGITUDE 0.0      // Your longitude (decimal degrees)
#define HOTSPOT_HEIGHT 0           // Antenna height in meters
#define HOTSPOT_LOCATION "Home"    // Location description
#define HOTSPOT_DESCRIPTION "ESP32 MMDVM Hotspot"

// ===== Network Settings =====
#define NETWORK_KEEPALIVE_INTERVAL 5000  // Keepalive interval in milliseconds
#define NETWORK_TIMEOUT 30000            // Network timeout in milliseconds
#define NETWORK_RECONNECT_DELAY 5000     // Delay before reconnecting

// ===== Debug Settings =====
#define DEBUG_SERIAL true          // Enable serial debug output
#define DEBUG_MMDVM false          // Enable MMDVM protocol debug
#define DEBUG_NETWORK false        // Enable network debug
#define DEBUG_DMR false            // Enable DMR protocol debug

// ===== Optional Features =====
#define ENABLE_OLED false          // Enable OLED display support
#define ENABLE_WEBSERVER false     // Enable web configuration interface
#define ENABLE_OTA false           // Enable over-the-air updates
#define ENABLE_MDNS true           // Enable mDNS (find hotspot by hostname)
#define MDNS_HOSTNAME "esp32-mmdvm" // mDNS hostname

// ===== Web Interface Settings =====
#define COPYRIGHT_TEXT "&copy; 2025 einstein.amsterdam"  // Footer copyright text

// ===== OTA Update Configuration =====
#define OTA_UPDATE_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/update.bin"
#define OTA_VERSION_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/version.txt"
#define OTA_TIMEOUT 30000          // OTA download timeout in milliseconds

// OLED Display Settings (if enabled)
#define OLED_I2C_ADDRESS 0x3C
#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// ===== Advanced Settings =====
// Buffer sizes
#define SERIAL_RX_BUFFER_SIZE 512
#define UDP_BUFFER_SIZE 512

// Timeouts
#define MMDVM_RESPONSE_TIMEOUT 1000  // Milliseconds to wait for MMDVM response
#define WIFI_CONNECT_TIMEOUT 30000   // Milliseconds to wait for WiFi connection

// ===== Multi-Protocol Support =====
// Enable/disable protocols (requires appropriate MMDVM firmware)
#define ENABLE_DMR true
#define ENABLE_DSTAR false
#define ENABLE_YSF false
#define ENABLE_P25 false
#define ENABLE_NXDN false
#define ENABLE_POCSAG false

// ===== Validation =====
#if !defined(DMR_CALLSIGN) || !defined(DMR_ID)
  #error "Please configure your DMR callsign and ID in config.h"
#endif

#if DMR_ID < 1000000 || DMR_ID > 9999999
  #error "DMR ID must be 7 digits"
#endif

#endif // CONFIG_H
