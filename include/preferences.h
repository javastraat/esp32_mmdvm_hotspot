/*
 * preferences.h - Configuration Storage for ESP32 MMDVM Hotspot
 *
 * Handles loading and saving configuration to/from ESP32 NVS (Non-Volatile Storage)
 *
 * NOTE: Function implementations remain in main .ino file
 * This header provides declarations and extern variables for configuration functions
 */

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

// External reference to Preferences object (defined in main .ino)
extern Preferences preferences;

// External configuration variables (defined in main .ino)
extern String dmr_callsign;
extern uint32_t dmr_id;
extern String dmr_server;
extern String dmr_password;
extern uint8_t dmr_essid;
extern uint32_t dmr_rx_freq;
extern uint32_t dmr_tx_freq;
extern uint8_t dmr_power;
extern uint8_t dmr_color_code;
extern float dmr_latitude;
extern float dmr_longitude;
extern int dmr_height;
extern String dmr_location;
extern String dmr_description;
extern String dmr_url;

// WiFi Network structure (defined in web/common/utils.h)
struct WiFiNetwork;  // Forward declaration
extern WiFiNetwork wifiNetworks[5];

extern String device_hostname;
extern bool verbose_logging;
extern bool debug_serial;
extern bool debug_mmdvm;
extern bool debug_network;
extern bool debug_dmr;
extern bool debug_password;
extern bool enable_oled;
extern bool oledAutoBlankEnabled;
extern unsigned long oledBlankTimeout;
extern long ntp_timezone_offset;
extern long ntp_daylight_offset;
extern String web_username;
extern String web_password;
extern bool mode_dmr_enabled;
extern bool mode_dstar_enabled;
extern bool mode_ysf_enabled;
extern bool mode_p25_enabled;
extern bool mode_nxdn_enabled;
extern bool mode_pocsag_enabled;
extern String modem_type;

// External logging function
extern void logSerial(String message);

// Function declarations
void loadConfig();
void saveConfig();

#endif // PREFERENCES_H
