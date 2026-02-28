/*
 * config.h - Factory Setup Configuration
 *
 * Minimal configuration for bootstrap firmware
 */

#ifndef FACTORY_CONFIG_H
#define FACTORY_CONFIG_H

// ===== Factory Firmware Version =====
#define FACTORY_VERSION "20260128_ESP32_SETUP"

// ===== Default WiFi Configuration =====
#define WIFI_SSID "TechInc"              // Default WiFi SSID
#define WIFI_PASSWORD "itoldyoualready"  // Default WiFi password
#define WIFI_TIMEOUT_SECONDS 20          // Seconds to wait for WiFi connection

// ===== Access Point Configuration =====
#define AP_SSID "ESP32-MMDVM-SETUP"      // AP SSID for factory setup
#define AP_PASSWORD "mmdvm1234"          // AP Password

// ===== OTA Update URLs =====
#define OTA_UPDATE_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/firmware/update.bin"
#define OTA_UPDATE_BETA_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/firmware/update_beta.bin"
#define OTA_VERSION_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/version.txt"
#define OTA_VERSION_BETA_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/version_beta.txt"
#define OTA_UPDATE_FACTORY_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/firmware/factory-setup.bin"
#define OTA_TIMEOUT 30000  // OTA download timeout in milliseconds

// ===== Footer Links =====
#define COPYRIGHT_TEXT "&copy; 2026 by PD2EMC & PD8JO"
#define FOOTER_LINK1_URL "https://einstein.amsterdam"
#define FOOTER_LINK1_TEXT "einstein.amsterdam"
#define FOOTER_LINK2_URL "https://pd8jo.nl"
#define FOOTER_LINK2_TEXT "pd8jo.nl"
#define FOOTER_LINK3_URL "https://github.com/javastraat/esp32_mmdvm_hotspot"
#define FOOTER_LINK3_TEXT "GitHub Project"

//#define LILYGO_T_ETH_ELITE_ESP32S3_MMDVM

#if defined(LILYGO_T_ETH_ELITE_ESP32S3_MMDVM)
// Network Pins
#define ETH_MISO_PIN 47
#define ETH_MOSI_PIN 21
#define ETH_SCLK_PIN 48
#define ETH_CS_PIN 45
#define ETH_INT_PIN 14
#define ETH_RST_PIN -1
#define ETH_ADDR 1

// ===== MMDVM Pin Configuration =====
#define MMDVM_SERIAL_BAUD 115200
#define MMDVM_RX_PIN 44          // ESP32 RX from MMDVM TX
#define MMDVM_TX_PIN 43          // ESP32 TX to MMDVM RX
#define MMDVM_BOOT0_PIN 4        // BOOT0 pin for bootloader control
#define MMDVM_RESET_PIN 13       // RESET pin for modem reset

// ===== OLED Display Pins =====
#define OLED_SDA_PIN 17          // I2C SDA for OLED
#define OLED_SCL_PIN 18          // I2C SCL for OLED
#define OLED_BUTTON_PIN 0        // Button to toggle OLED display (directly from main config)

#else
// Default pins for generic ESP32 boards
#define OLED_SDA_PIN 21          // Default I2C SDA
#define OLED_SCL_PIN 22          // Default I2C SCL
#define OLED_BUTTON_PIN 0        // Button to toggle OLED display

#endif

// ===== OLED Display Settings =====
#define ENABLE_OLED true         // Enable OLED display support
#define OLED_I2C_ADDRESS 0x3C    // I2C address for OLED display (0x3C or 0x3D)
#define OLED_WIDTH 128           // OLED display width
#define OLED_HEIGHT 64           // OLED display height

// ===== MMDVM Pin Configuration =====
#define MMDVM_SERIAL_BAUD 115200
#define MMDVM_RX_PIN 44          // ESP32 RX from MMDVM TX
#define MMDVM_TX_PIN 43          // ESP32 TX to MMDVM RX
#define MMDVM_BOOT0_PIN 4        // BOOT0 pin for bootloader control
#define MMDVM_RESET_PIN 13       // RESET pin for modem reset

#endif // FACTORY_CONFIG_H
