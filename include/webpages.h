/*
 * webpages.h - Web Interface Pages for ESP32 MMDVM Hotspot
 *
 * This file has been refactored into modular components for better maintainability.
 * All page handlers are now organized in the web/ subdirectory.
 */

#ifndef WEBPAGES_H
#define WEBPAGES_H

#include <WebServer.h>
#include <Preferences.h>

// External constants
#define SERIAL_LOG_SIZE 50

// External variables needed by web pages
extern WebServer server;
extern bool wifiConnected;
extern bool apMode;
extern bool mmdvmReady;
extern bool dmrLoggedIn;
extern uint32_t currentTalkgroup;
extern String dmrLoginStatus;
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
extern String altSSID;
extern String altPassword;
extern String device_hostname;
extern bool verbose_logging;
extern String web_username;
extern String web_password;
extern String serialLog[SERIAL_LOG_SIZE];
extern int serialLogIndex;
extern Preferences preferences;
extern String firmwareVersion;

// Mode enable/disable settings
extern bool mode_dmr_enabled;
extern bool mode_dstar_enabled;
extern bool mode_ysf_enabled;
extern bool mode_p25_enabled;
extern bool mode_nxdn_enabled;
extern bool mode_pocsag_enabled;

// External functions
extern void logSerial(String message);
extern void saveConfig();

// ===== Include Modular Web Components =====

// Common components (CSS, navigation, utilities)
#include "web/common/css.h"
#include "web/common/navigation.h"
#include "web/common/utils.h"

// Page handlers
#include "web/pages/home.h"
#include "web/pages/monitor.h"
#include "web/pages/status.h"
#include "web/pages/wifi_config.h"
#include "web/pages/mode_config.h"
#include "web/pages/admin.h"

#endif  // WEBPAGES_H
