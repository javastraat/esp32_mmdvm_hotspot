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
#define OTA_UPDATE_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/update.bin"
#define OTA_UPDATE_BETA_URL "https://raw.githubusercontent.com/javastraat/esp32_mmdvm_hotspot/refs/heads/main/update_beta.bin"
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

#endif // FACTORY_CONFIG_H
