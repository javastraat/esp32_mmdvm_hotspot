/*
 * config.h - Factory Setup Configuration
 *
 * Minimal configuration for bootstrap firmware
 */

#ifndef FACTORY_CONFIG_H
#define FACTORY_CONFIG_H

// ===== Factory Firmware Version =====
#define FACTORY_VERSION "FACTORY_SETUP_v1.0"

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
#define OTA_TIMEOUT 30000  // OTA download timeout in milliseconds

// ===== Footer Links =====
#define COPYRIGHT_TEXT "&copy; 2025 by PD2EMC & PD8JO"
#define FOOTER_LINK1_URL "https://einstein.amsterdam"
#define FOOTER_LINK1_TEXT "einstein.amsterdam"
#define FOOTER_LINK2_URL "https://pd8jo.nl"
#define FOOTER_LINK2_TEXT "pd8jo.nl"
#define FOOTER_LINK3_URL "https://github.com/javastraat/esp32_mmdvm_hotspot"
#define FOOTER_LINK3_TEXT "GitHub Project"

#define LILYGO_T_ETH_ELITE_ESP32S3_MMDVM

#if defined(LILYGO_T_ETH_ELITE_ESP32S3_MMDVM)
// Network Pins
#define ETH_MISO_PIN 47
#define ETH_MOSI_PIN 21
#define ETH_SCLK_PIN 48
#define ETH_CS_PIN 45
#define ETH_INT_PIN 14
#define ETH_RST_PIN -1
#define ETH_ADDR 1

#endif

#endif // FACTORY_CONFIG_H
