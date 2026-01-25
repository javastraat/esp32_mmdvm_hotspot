/*
 * system_handlers.h - System Administration Handlers for ESP32 MMDVM Hotspot
 *
 * Contains handlers for:
 * - System reboot and restart services
 * - Configuration reset, export, import
 * - Preferences management and repair
 * - Hostname configuration
 */

#ifndef SYSTEM_HANDLERS_H
#define SYSTEM_HANDLERS_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include "nvs_flash.h"

// External variables
extern WebServer server;
extern Preferences preferences;
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
extern void connectToDMRNetwork();
extern bool dmrLoggedIn;
extern int loginAttempts;
extern WiFiClient mqttWiFiClient;
extern PubSubClient mqttClient;
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
extern bool mqtt_enabled;
extern String mqtt_broker;
extern uint16_t mqtt_port;
extern String mqtt_username;
extern String mqtt_password;
extern String mqtt_client_id;
extern String mqtt_topic_prefix;
extern uint32_t mqtt_publish_interval;

// External functions
extern bool checkAuthentication();
extern void logSerial(String message);
extern void saveConfig();
extern String getCommonCSS();
extern String getNavigation(String activePage);
extern String getFooter();

// ===== System Reset Handlers =====

void handleResetConfig() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Complete Storage Reset - ESP32 MMDVM</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".container { max-width: 600px; margin: 40px auto; padding: 0; text-align: center; }";
  html += ".card { text-align: center; }";
  html += "h1 { color: #dc3545; border-bottom: 2px solid #dc3545; padding-bottom: 10px; margin-bottom: 20px; }";
  html += ".warning { padding: 15px; background: rgba(255, 193, 7, 0.15); border-left: 4px solid #ffc107; margin: 20px 0; text-align: left; border-radius: 4px; }";
  html += ".danger { padding: 15px; background: rgba(220, 53, 69, 0.15); border-left: 4px solid #dc3545; margin: 20px 0; text-align: left; border-radius: 4px; }";
  html += ".nav { margin: 20px 0; }";
  html += ".nav a { display: inline-block; padding: 10px 20px; margin: 5px; background: #007bff; color: white; text-decoration: none; border-radius: 4px; }";
  html += ".nav a:hover { background: #0056b3; }";
  html += ".btn-danger { background: #dc3545; color: white; padding: 12px 30px; border: none; border-radius: 4px; cursor: pointer; margin: 10px; font-size: 16px; text-decoration: none; display: inline-block; }";
  html += ".btn-danger:hover { background: #c82333; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<div class='card'>";
  html += "<h1>Complete Storage Reset</h1>";
  html += "<div class='danger'>";
  html += "<strong>EXTREME WARNING!</strong><br>";
  html += "This will completely erase ALL ESP32 flash storage including:<br>";
  html += "- DMR configuration (callsign, ID, server, ESSID, frequencies)<br>";
  html += "- All WiFi credentials (primary and alternate)<br>";
  html += "- Location and RF settings<br>";
  html += "- ALL stored preferences in ANY namespace<br>";
  html += "- Complete NVS (Non-Volatile Storage) partition<br>";
  html += "- Any other data stored by any application<br>";
  html += "</div>";
  html += "<div class='warning'>";
  html += "<strong>After reset:</strong><br>";
  html += "- Device will restart with factory defaults from config.h<br>";
  html += "- You will need to reconfigure ALL settings<br>";
  html += "- This action cannot be undone!<br>";
  html += "</div>";
  html += "<p><strong>Are you absolutely sure you want to erase ALL storage?</strong></p>";
  html += "<p style='color: #dc3545; font-weight: bold;'>This will reset the ESP32 to completely factory state!</p>";
  html += "<form action='/confirmreset' method='POST' onsubmit='return confirmReset()'>";
  html += "<button type='submit' class='btn-danger'>Yes, Erase Everything!</button>";
  html += "</form>";
  html += "<div class='nav'><a href='/admin'>Cancel & Go Back to Admin</a></div>";
  html += "<script>";
  html += "function confirmReset() {";
  html += "  return confirm('FINAL WARNING!\\n\\nYou are about to PERMANENTLY ERASE all settings and data.\\n\\nThis action CANNOT be undone!\\n\\nPress OK to proceed with complete storage reset, or Cancel to abort.');";
  html += "}";
  html += "</script>";
  html += "</div>";
  html += getFooter();
  html += "</div></body></html>";

  server.send(200, "text/html; charset=UTF-8", html);
}

void handleConfirmReset() {
  if (!checkAuthentication()) return;

  // Clear ALL ESP32 flash storage (not just our namespace)
  logSerial("Starting complete ESP32 flash storage reset...");

  // Method 1: Clear known namespaces
  const char* knownNamespaces[] = {"mmdvm", "wifi", "nvs", "app", "system", "user", "config", "settings"};
  int namespaceCount = sizeof(knownNamespaces) / sizeof(knownNamespaces[0]);

  for (int i = 0; i < namespaceCount; i++) {
    preferences.begin(knownNamespaces[i], false);
    if (preferences.clear()) {
      logSerial("Cleared namespace: " + String(knownNamespaces[i]));
    }
    preferences.end();
    delay(10); // Small delay between operations
  }

  // Method 2: Use ESP32 NVS erase (more thorough)
  // This erases the entire NVS partition
  esp_err_t err = nvs_flash_erase();
  if (err == ESP_OK) {
    logSerial("NVS flash partition completely erased");
    // Reinitialize NVS after erase
    err = nvs_flash_init();
    if (err == ESP_OK) {
      logSerial("NVS reinitialized successfully");
    } else {
      logSerial("NVS reinitialize failed: " + String(esp_err_to_name(err)));
    }
  } else {
    logSerial("NVS erase failed: " + String(esp_err_to_name(err)));
  }

  // Method 3: Clear WiFi credentials stored by ESP32 WiFi library
  WiFi.disconnect(true, true);  // Disconnect and erase WiFi credentials from flash
  logSerial("WiFi credentials erased from flash");

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Complete Storage Reset</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".container { max-width: 600px; margin: 40px auto; padding: 0; text-align: center; }";
  html += ".card { text-align: center; }";
  html += "h1 { color: #28a745; margin-bottom: 20px; }";
  html += ".info { text-align: left; margin: 20px 0; padding: 15px; background: var(--info-bg); border-left: 4px solid #007bff; border-radius: 4px; }";
  html += ".warning { text-align: left; margin: 20px 0; padding: 15px; background: rgba(255, 193, 7, 0.15); border-left: 4px solid #ffc107; border-radius: 4px; }";
  html += ".countdown { font-size: 24px; font-weight: bold; color: #007bff; margin: 20px 0; }";
  html += ".spinner { border: 4px solid var(--border-color); border-top: 4px solid #007bff; border-radius: 50%; width: 40px; height: 40px; animation: spin 1s linear infinite; margin: 20px auto; }";
  html += ".btn { display: inline-block; padding: 12px 24px; margin: 10px; background: #007bff; color: white; text-decoration: none; border-radius: 4px; font-weight: bold; }";
  html += ".btn:hover { background: #0056b3; }";
  html += "#reconnectBtn { display: none; }";
  html += "@keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<div class='card'>";
  html += "<h1>Complete Storage Reset!</h1>";
  html += "<div class='info'>";
  html += "<strong>What was cleared:</strong><br>";
  html += "- All ESP32 Preferences namespaces<br>";
  html += "- Complete NVS (Non-Volatile Storage) partition<br>";
  html += "- WiFi credentials stored by ESP32 WiFi library<br>";
  html += "- All DMR and system settings<br>";
  html += "- Any other stored configuration data<br>";
  html += "</div>";
  html += "<div class='spinner' id='spinner'></div>";
  html += "<div class='countdown' id='countdown'>Restarting in 5 seconds...</div>";
  html += "<div class='warning' id='instructions' style='display:none;'>";
  html += "<strong>Device has restarted!</strong><br>";
  html += "The ESP32 is now running with factory defaults.<br><br>";
  html += "Click the button below to reload and access the configuration interface.";
  html += "</div>";
  html += "<a href='/' class='btn' id='reconnectBtn'>Reload Page</a>";
  html += "<script>";
  html += "var timeLeft = 5;";
  html += "var countdown = setInterval(function() {";
  html += "  timeLeft--;";
  html += "  if (timeLeft > 0) {";
  html += "    document.getElementById('countdown').innerHTML = 'Restarting in ' + timeLeft + ' second' + (timeLeft != 1 ? 's' : '') + '...';";
  html += "  } else {";
  html += "    document.getElementById('countdown').innerHTML = 'Device is restarting...';";
  html += "    clearInterval(countdown);";
  html += "    setTimeout(function() {";
  html += "      document.getElementById('spinner').style.display = 'none';";
  html += "      document.getElementById('countdown').style.display = 'none';";
  html += "      document.getElementById('instructions').style.display = 'block';";
  html += "      document.getElementById('reconnectBtn').style.display = 'inline-block';";
  html += "    }, 8000);";
  html += "  }";
  html += "}, 1000);";
  html += "</script>";
  html += "</div>";
  html += getFooter();
  html += "</div></body></html>";

  server.send(200, "text/html; charset=UTF-8", html);

  logSerial("Complete ESP32 storage reset completed - restarting...");
  delay(5000);
  ESP.restart();
}

// ===== Preferences Repair Handler =====

void handleCleanupPreferences() {
  logSerial("=== PREFERENCE REPAIR STARTING ===");
  logSerial("[REPAIR] Checking all 57 possible preference keys...");

  int missingCount = 0;
  int existingCount = 0;

  // Reopen preferences in read-write mode
  preferences.end();
  preferences.begin("mmdvm", false);

  // Check and add missing DMR Settings (15 possible)
  if (!preferences.isKey("dmr_callsign")) {
    preferences.putString("dmr_callsign", DMR_CALLSIGN);
    logSerial("[REPAIR] Added: dmr_callsign");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("dmr_id")) {
    preferences.putUInt("dmr_id", DMR_ID);
    logSerial("[REPAIR] Added: dmr_id");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("dmr_server")) {
    preferences.putString("dmr_server", DMR_SERVER);
    logSerial("[REPAIR] Added: dmr_server");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("dmr_password")) {
    preferences.putString("dmr_password", DMR_PASSWORD);
    logSerial("[REPAIR] Added: dmr_password");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("dmr_essid")) {
    preferences.putUChar("dmr_essid", 0);
    logSerial("[REPAIR] Added: dmr_essid");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("dmr_rx_freq")) {
    preferences.putUInt("dmr_rx_freq", 434000000);
    logSerial("[REPAIR] Added: dmr_rx_freq");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("dmr_tx_freq")) {
    preferences.putUInt("dmr_tx_freq", 434000000);
    logSerial("[REPAIR] Added: dmr_tx_freq");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("dmr_power")) {
    preferences.putUChar("dmr_power", 10);
    logSerial("[REPAIR] Added: dmr_power");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("dmr_cc")) {
    preferences.putUChar("dmr_cc", DMR_COLORCODE);
    logSerial("[REPAIR] Added: dmr_cc");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("dmr_lat")) {
    preferences.putFloat("dmr_lat", 0.0);
    logSerial("[REPAIR] Added: dmr_lat");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("dmr_lon")) {
    preferences.putFloat("dmr_lon", 0.0);
    logSerial("[REPAIR] Added: dmr_lon");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("dmr_height")) {
    preferences.putInt("dmr_height", 0);
    logSerial("[REPAIR] Added: dmr_height");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("dmr_location")) {
    preferences.putString("dmr_location", DMR_LOCATION);
    logSerial("[REPAIR] Added: dmr_location");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("dmr_desc")) {
    preferences.putString("dmr_desc", DMR_DESCRIPTION);
    logSerial("[REPAIR] Added: dmr_desc");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("dmr_url")) {
    preferences.putString("dmr_url", DMR_URL);
    logSerial("[REPAIR] Added: dmr_url");
    missingCount++;
  } else existingCount++;

  // Check WiFi Networks (15 possible)
  String defaultLabels[] = {WIFI_SLOT1_LABEL, WIFI_SLOT2_LABEL, WIFI_SLOT3_LABEL, WIFI_SLOT4_LABEL, WIFI_SLOT5_LABEL};
  for (int i = 0; i < 5; i++) {
    String labelKey = "wifi" + String(i) + "_label";
    String ssidKey = "wifi" + String(i) + "_ssid";
    String passKey = "wifi" + String(i) + "_pass";

    if (!preferences.isKey(labelKey.c_str())) {
      preferences.putString(labelKey.c_str(), defaultLabels[i]);
      logSerial("[REPAIR] Added: " + labelKey);
      missingCount++;
    } else existingCount++;

    if (!preferences.isKey(ssidKey.c_str())) {
      preferences.putString(ssidKey.c_str(), "");
      logSerial("[REPAIR] Added: " + ssidKey);
      missingCount++;
    } else existingCount++;

    if (!preferences.isKey(passKey.c_str())) {
      preferences.putString(passKey.c_str(), "");
      logSerial("[REPAIR] Added: " + passKey);
      missingCount++;
    } else existingCount++;
  }

  // Check System Settings (11 possible)
  if (!preferences.isKey("hostname")) {
    preferences.putString("hostname", MDNS_HOSTNAME);
    logSerial("[REPAIR] Added: hostname");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("verbose_log")) {
    preferences.putBool("verbose_log", false);
    logSerial("[REPAIR] Added: verbose_log");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("debug_serial")) {
    preferences.putBool("debug_serial", DEBUG_SERIAL);
    logSerial("[REPAIR] Added: debug_serial");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("debug_mmdvm")) {
    preferences.putBool("debug_mmdvm", DEBUG_MMDVM);
    logSerial("[REPAIR] Added: debug_mmdvm");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("debug_network")) {
    preferences.putBool("debug_network", DEBUG_NETWORK);
    logSerial("[REPAIR] Added: debug_network");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("debug_dmr")) {
    preferences.putBool("debug_dmr", DEBUG_DMR);
    logSerial("[REPAIR] Added: debug_dmr");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("debug_password")) {
    preferences.putBool("debug_password", DEBUG_PASSWORD);
    logSerial("[REPAIR] Added: debug_password");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("enable_oled")) {
    preferences.putBool("enable_oled", ENABLE_OLED);
    logSerial("[REPAIR] Added: enable_oled");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("oled_autoblank")) {
    preferences.putBool("oled_autoblank", false);
    logSerial("[REPAIR] Added: oled_autoblank");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("oled_blank_to")) {
    preferences.putULong("oled_blank_to", 60000);
    logSerial("[REPAIR] Added: oled_blank_to");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("modem_type")) {
    preferences.putString("modem_type", DEFAULT_MODEM_TYPE);
    logSerial("[REPAIR] Added: modem_type");
    missingCount++;
  } else existingCount++;

  // Check Mode Settings (6 possible)
  if (!preferences.isKey("mode_dmr")) {
    preferences.putBool("mode_dmr", DEFAULT_MODE_DMR);
    logSerial("[REPAIR] Added: mode_dmr");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("mode_dstar")) {
    preferences.putBool("mode_dstar", DEFAULT_MODE_DSTAR);
    logSerial("[REPAIR] Added: mode_dstar");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("mode_ysf")) {
    preferences.putBool("mode_ysf", DEFAULT_MODE_YSF);
    logSerial("[REPAIR] Added: mode_ysf");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("mode_p25")) {
    preferences.putBool("mode_p25", DEFAULT_MODE_P25);
    logSerial("[REPAIR] Added: mode_p25");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("mode_nxdn")) {
    preferences.putBool("mode_nxdn", DEFAULT_MODE_NXDN);
    logSerial("[REPAIR] Added: mode_nxdn");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("mode_pocsag")) {
    preferences.putBool("mode_pocsag", DEFAULT_MODE_POCSAG);
    logSerial("[REPAIR] Added: mode_pocsag");
    missingCount++;
  } else existingCount++;

  // Check Web Auth (2 possible)
  if (!preferences.isKey("web_username")) {
    preferences.putString("web_username", WEB_USERNAME);
    logSerial("[REPAIR] Added: web_username");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("web_password")) {
    preferences.putString("web_password", WEB_PASSWORD);
    logSerial("[REPAIR] Added: web_password");
    missingCount++;
  } else existingCount++;

  // Check MQTT Settings (8 possible)
  if (!preferences.isKey("mqtt_enabled")) {
    preferences.putBool("mqtt_enabled", MQTT_ENABLED);
    logSerial("[REPAIR] Added: mqtt_enabled");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("mqtt_broker")) {
    preferences.putString("mqtt_broker", MQTT_BROKER);
    logSerial("[REPAIR] Added: mqtt_broker");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("mqtt_port")) {
    preferences.putUShort("mqtt_port", MQTT_PORT);
    logSerial("[REPAIR] Added: mqtt_port");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("mqtt_user")) {
    preferences.putString("mqtt_user", MQTT_USERNAME);
    logSerial("[REPAIR] Added: mqtt_user");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("mqtt_pass")) {
    preferences.putString("mqtt_pass", MQTT_PASSWORD);
    logSerial("[REPAIR] Added: mqtt_pass");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("mqtt_client")) {
    preferences.putString("mqtt_client", MQTT_CLIENT_ID);
    logSerial("[REPAIR] Added: mqtt_client");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("mqtt_prefix")) {
    preferences.putString("mqtt_prefix", MQTT_TOPIC_PREFIX);
    logSerial("[REPAIR] Added: mqtt_prefix");
    missingCount++;
  } else existingCount++;

  if (!preferences.isKey("mqtt_interval")) {
    preferences.putUInt("mqtt_interval", MQTT_PUBLISH_INTERVAL);
    logSerial("[REPAIR] Added: mqtt_interval");
    missingCount++;
  } else existingCount++;

  preferences.end();

  // Summary
  logSerial("=== PREFERENCE REPAIR COMPLETE ===");
  logSerial("[REPAIR] Existing: " + String(existingCount) + "/57, Added: " + String(missingCount) + "/57");
  logSerial("[REPAIR] Total after repair: " + String(existingCount + missingCount) + "/57");

  if (missingCount == 0) {
    logSerial("[REPAIR] All preferences intact - no repair needed!");
    server.send(200, "text/plain", "All preferences intact! No repair needed. (" + String(existingCount) + "/57)");
  } else {
    logSerial("[REPAIR] Successfully repaired - rebooting...");
    server.send(200, "text/plain", "Repaired " + String(missingCount) + " missing preferences! Rebooting...");
    delay(1000);
    ESP.restart();
  }
}

// ===== Hostname Handler =====

void handleSaveHostname() {
  if (!checkAuthentication()) return;

  if (server.hasArg("hostname")) {
    String newHostname = server.arg("hostname");

    // Validate hostname
    if (newHostname.length() > 0 && newHostname.length() <= 32) {
      device_hostname = newHostname;
      saveConfig();

      server.send(200, "text/plain", "SUCCESS: Hostname saved");
      logSerial("Hostname changed to: " + device_hostname);

      delay(1000);
      ESP.restart();
    } else {
      server.send(400, "text/plain", "ERROR: Invalid hostname length");
    }
  } else {
    server.send(400, "text/plain", "ERROR: Missing hostname parameter");
  }
}

// ===== System Control Handlers =====

void handleReboot() {
  if (!checkAuthentication()) return;

  server.send(200, "text/plain", "Rebooting...");
  logSerial("System reboot requested");
  delay(1000);
  ESP.restart();
}

void handleRestartDMR() {
  if (!checkAuthentication()) return;

  logSerial("[USER] Restarting DMR connection...");
  
  // Disconnect from DMR (state will be managed by connectToDMRNetwork)
  dmrLoggedIn = false;
  loginAttempts = 0;
  
  // Trigger immediate reconnection
  connectToDMRNetwork();
  
  server.send(200, "text/plain", "DMR reconnection initiated");
  logSerial("[USER] DMR reconnection initiated");
}

void handleRestartMQTT() {
  if (!checkAuthentication()) return;

  logSerial("[USER] Restarting MQTT connection...");
  
  // Disconnect from MQTT
  if (mqttClient.connected()) {
    mqttClient.disconnect();
  }
  
  // Reconnection will happen automatically in main loop
  
  server.send(200, "text/plain", "MQTT reconnection initiated");
  logSerial("[USER] MQTT reconnection initiated");
}

void handleRestartServices() {
  if (!checkAuthentication()) return;

  logSerial("[USER] Restarting all network services...");
  
  // Restart DMR (state will be managed by connectToDMRNetwork)
  dmrLoggedIn = false;
  loginAttempts = 0;
  connectToDMRNetwork();
  
  // Restart MQTT
  if (mqttClient.connected()) {
    mqttClient.disconnect();
  }
  
  server.send(200, "text/plain", "All services restarted");
  logSerial("[USER] DMR and MQTT reconnection initiated");
}

// ===== Config Export/Import Handlers =====

void handleExportConfig() {
  String config = "# ESP32 MMDVM Hotspot Configuration Export (Complete)\n";
  config += "# Generated on: " + String(__DATE__) + " " + String(__TIME__) + "\n";
  config += "# WARNING: This file contains passwords - keep secure!\n\n";

  // DMR Configuration
  config += "[DMR_CONFIG]\n";
  config += "DMR_CALLSIGN=" + dmr_callsign + "\n";
  config += "DMR_ID=" + String(dmr_id) + "\n";
  config += "DMR_SERVER=" + dmr_server + "\n";
  config += "DMR_PASSWORD=" + dmr_password + "\n";
  config += "DMR_ESSID=" + String(dmr_essid) + "\n";
  config += "DMR_RX_FREQ=" + String(dmr_rx_freq) + "\n";
  config += "DMR_TX_FREQ=" + String(dmr_tx_freq) + "\n";
  config += "DMR_POWER=" + String(dmr_power) + "\n";
  config += "DMR_COLOR_CODE=" + String(dmr_color_code) + "\n";
  config += "DMR_LATITUDE=" + String(dmr_latitude, 6) + "\n";
  config += "DMR_LONGITUDE=" + String(dmr_longitude, 6) + "\n";
  config += "DMR_HEIGHT=" + String(dmr_height) + "\n";
  config += "DMR_LOCATION=" + dmr_location + "\n";
  config += "DMR_DESCRIPTION=" + dmr_description + "\n";
  config += "DMR_URL=" + dmr_url + "\n";

  // WiFi Configuration
  config += "\n[WIFI_CONFIG]\n";
  for (int i = 0; i < 5; i++) {
    config += "WIFI" + String(i) + "_LABEL=" + wifiNetworks[i].label + "\n";
    config += "WIFI" + String(i) + "_SSID=" + wifiNetworks[i].ssid + "\n";
    config += "WIFI" + String(i) + "_PASSWORD=" + wifiNetworks[i].password + "\n";
  }

  // System Configuration
  config += "\n[SYSTEM_CONFIG]\n";
  config += "HOSTNAME=" + device_hostname + "\n";
  config += "VERBOSE_LOGGING=" + String(verbose_logging ? "1" : "0") + "\n";
  config += "DEBUG_SERIAL=" + String(debug_serial ? "1" : "0") + "\n";
  config += "DEBUG_MMDVM=" + String(debug_mmdvm ? "1" : "0") + "\n";
  config += "DEBUG_NETWORK=" + String(debug_network ? "1" : "0") + "\n";
  config += "DEBUG_DMR=" + String(debug_dmr ? "1" : "0") + "\n";
  config += "DEBUG_PASSWORD=" + String(debug_password ? "1" : "0") + "\n";
  config += "ENABLE_OLED=" + String(enable_oled ? "1" : "0") + "\n";
  config += "OLED_AUTO_BLANK=" + String(oledAutoBlankEnabled ? "1" : "0") + "\n";
  config += "OLED_BLANK_TIMEOUT=" + String(oledBlankTimeout) + "\n";
  config += "NTP_TIMEZONE_OFFSET=" + String(ntp_timezone_offset) + "\n";
  config += "NTP_DAYLIGHT_OFFSET=" + String(ntp_daylight_offset) + "\n";
  config += "WEB_USERNAME=" + web_username + "\n";
  config += "WEB_PASSWORD=" + web_password + "\n";
  config += "MODEM_TYPE=" + modem_type + "\n";

  // Mode Configuration
  config += "\n[MODE_CONFIG]\n";
  config += "MODE_DMR=" + String(mode_dmr_enabled ? "1" : "0") + "\n";
  config += "MODE_DSTAR=" + String(mode_dstar_enabled ? "1" : "0") + "\n";
  config += "MODE_YSF=" + String(mode_ysf_enabled ? "1" : "0") + "\n";
  config += "MODE_P25=" + String(mode_p25_enabled ? "1" : "0") + "\n";
  config += "MODE_NXDN=" + String(mode_nxdn_enabled ? "1" : "0") + "\n";
  config += "MODE_POCSAG=" + String(mode_pocsag_enabled ? "1" : "0") + "\n";

  // MQTT Configuration
  config += "\n[MQTT_CONFIG]\n";
  config += "MQTT_ENABLED=" + String(mqtt_enabled ? "1" : "0") + "\n";
  config += "MQTT_BROKER=" + mqtt_broker + "\n";
  config += "MQTT_PORT=" + String(mqtt_port) + "\n";
  config += "MQTT_USERNAME=" + mqtt_username + "\n";
  config += "MQTT_PASSWORD=" + mqtt_password + "\n";
  config += "MQTT_CLIENT_ID=" + mqtt_client_id + "\n";
  config += "MQTT_TOPIC_PREFIX=" + mqtt_topic_prefix + "\n";
  config += "MQTT_PUBLISH_INTERVAL=" + String(mqtt_publish_interval) + "\n";

  server.send(200, "text/plain", config);
}

void handleImportConfig() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    logSerial("Starting configuration import: " + upload.filename);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    // Process configuration data chunk by chunk
    String chunk = "";
    for (size_t i = 0; i < upload.currentSize; i++) {
      chunk += (char)upload.buf[i];
    }

    // Parse configuration lines
    int lineStart = 0;
    int lineEnd = chunk.indexOf('\n');

    while (lineEnd != -1) {
      String line = chunk.substring(lineStart, lineEnd);
      line.trim();

      // Skip comments and empty lines
      if (line.length() > 0 && !line.startsWith("#") && !line.startsWith("[")) {
        int equalPos = line.indexOf('=');
        if (equalPos > 0) {
          String key = line.substring(0, equalPos);
          String value = line.substring(equalPos + 1);

          // Apply configuration values
          if (key == "DMR_CALLSIGN") dmr_callsign = value;
          else if (key == "DMR_ID") dmr_id = value.toInt();
          else if (key == "DMR_SERVER") dmr_server = value;
          else if (key == "DMR_PASSWORD") dmr_password = value;
          else if (key == "DMR_ESSID") dmr_essid = value.toInt();
          else if (key == "DMR_RX_FREQ") dmr_rx_freq = value.toInt();
          else if (key == "DMR_TX_FREQ") dmr_tx_freq = value.toInt();
          else if (key == "DMR_POWER") dmr_power = value.toInt();
          else if (key == "DMR_COLOR_CODE") dmr_color_code = value.toInt();
          else if (key == "DMR_LATITUDE") dmr_latitude = value.toFloat();
          else if (key == "DMR_LONGITUDE") dmr_longitude = value.toFloat();
          else if (key == "DMR_HEIGHT") dmr_height = value.toInt();
          else if (key == "DMR_LOCATION") dmr_location = value;
          else if (key == "DMR_DESCRIPTION") dmr_description = value;
          else if (key == "DMR_URL") dmr_url = value;
          // WiFi networks (5 slots)
          else if (key.startsWith("WIFI") && key.indexOf("_LABEL") > 0) {
            int slot = key.substring(4, key.indexOf("_LABEL")).toInt();
            if (slot >= 0 && slot < 5) wifiNetworks[slot].label = value;
          }
          else if (key.startsWith("WIFI") && key.indexOf("_SSID") > 0) {
            int slot = key.substring(4, key.indexOf("_SSID")).toInt();
            if (slot >= 0 && slot < 5) wifiNetworks[slot].ssid = value;
          }
          else if (key.startsWith("WIFI") && key.indexOf("_PASSWORD") > 0) {
            int slot = key.substring(4, key.indexOf("_PASSWORD")).toInt();
            if (slot >= 0 && slot < 5) wifiNetworks[slot].password = value;
          }
          else if (key == "ALT_SSID") wifiNetworks[0].ssid = value;  // Legacy support
          else if (key == "ALT_PASSWORD") wifiNetworks[0].password = value;  // Legacy support
          else if (key == "HOSTNAME") device_hostname = value;
          else if (key == "VERBOSE_LOGGING") verbose_logging = (value == "1");
          else if (key == "DEBUG_SERIAL") debug_serial = (value == "1");
          else if (key == "DEBUG_MMDVM") debug_mmdvm = (value == "1");
          else if (key == "DEBUG_NETWORK") debug_network = (value == "1");
          else if (key == "DEBUG_DMR") debug_dmr = (value == "1");
          else if (key == "DEBUG_PASSWORD") debug_password = (value == "1");
          else if (key == "ENABLE_OLED") enable_oled = (value == "1");
          else if (key == "OLED_AUTO_BLANK") oledAutoBlankEnabled = (value == "1");
          else if (key == "OLED_BLANK_TIMEOUT") oledBlankTimeout = value.toInt();
          else if (key == "NTP_TIMEZONE_OFFSET") ntp_timezone_offset = value.toInt();
          else if (key == "NTP_DAYLIGHT_OFFSET") ntp_daylight_offset = value.toInt();
          else if (key == "WEB_USERNAME") web_username = value;
          else if (key == "WEB_PASSWORD") web_password = value;
          else if (key == "MODEM_TYPE") modem_type = value;
          else if (key == "MODE_DMR") mode_dmr_enabled = (value == "1");
          else if (key == "MODE_DSTAR") mode_dstar_enabled = (value == "1");
          else if (key == "MODE_YSF") mode_ysf_enabled = (value == "1");
          else if (key == "MODE_P25") mode_p25_enabled = (value == "1");
          else if (key == "MODE_NXDN") mode_nxdn_enabled = (value == "1");
          else if (key == "MODE_POCSAG") mode_pocsag_enabled = (value == "1");
          else if (key == "MQTT_ENABLED") mqtt_enabled = (value == "1");
          else if (key == "MQTT_BROKER") mqtt_broker = value;
          else if (key == "MQTT_PORT") mqtt_port = value.toInt();
          else if (key == "MQTT_USERNAME") mqtt_username = value;
          else if (key == "MQTT_PASSWORD") mqtt_password = value;
          else if (key == "MQTT_CLIENT_ID") mqtt_client_id = value;
          else if (key == "MQTT_TOPIC_PREFIX") mqtt_topic_prefix = value;
          else if (key == "MQTT_PUBLISH_INTERVAL") mqtt_publish_interval = value.toInt();

          logSerial("Imported: " + key + " = " + value);
        }
      }

      lineStart = lineEnd + 1;
      lineEnd = chunk.indexOf('\n', lineStart);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    logSerial("Configuration import completed, saving to NVS...");

    // Save all imported settings to NVS
    saveConfig();

    logSerial("Configuration import successful: " + String(upload.totalSize) + " bytes processed");
    server.send(200, "text/plain", "SUCCESS: Configuration imported and saved");

    // Reboot after successful import
    delay(2000);
    ESP.restart();
  }
}

// ===== Show Preferences Handler =====

void handleShowPreferences() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>" + dmr_callsign + " - ESP32 MMDVM Hotspot</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".pref-table { width: 100%; border-collapse: collapse; margin: 0; }";
  html += ".pref-table th, .pref-table td { padding: 12px; text-align: left; border-bottom: 1px solid var(--border-color); }";
  html += ".pref-table tbody tr:last-child td { border-bottom: none; }";
  html += ".pref-table th { background-color: var(--hover-bg); font-weight: bold; color: var(--text-color); }";
  html += ".pref-table tr:nth-child(even) { background-color: var(--hover-bg); }";
  html += ".pref-key { font-family: 'Courier New', monospace; color: var(--primary-color); }";
  html += ".pref-value { font-family: 'Courier New', monospace; word-break: break-all; color: var(--text-color); }";
  html += ".pref-type { color: var(--text-muted); font-size: 0.9em; }";
  html += ".password-container { display: inline-flex; align-items: center; gap: 8px; }";
  html += ".password-toggle { cursor: pointer; font-size: 16px; color: var(--primary-color); user-select: none; }";
  html += ".password-toggle:hover { color: var(--primary-hover); }";
  html += ".password-hidden { color: var(--text-muted); }";
  html += ".pref-category { margin: 25px 0; border: 1px solid var(--border-color); border-radius: 8px; overflow: hidden; background: var(--card-bg); }";
  html += ".pref-category-header { padding: 15px 20px; background: var(--primary-color); color: white; cursor: pointer; user-select: none; display: flex; justify-content: space-between; align-items: center; font-weight: bold; font-size: 1.1em; }";
  html += ".pref-category-header:hover { background: var(--primary-hover); }";
  html += ".pref-category-content { display: block; }";
  html += ".pref-category-content.collapsed { display: none; }";
  html += ".pref-category-toggle { font-size: 1.2em; transition: transform 0.3s; }";
  html += ".pref-category-toggle.collapsed { transform: rotate(-90deg); }";
  html += ".pref-category .pref-table { margin: 0; }";
  html += ".category-summary { padding: 10px 20px; background: var(--hover-bg); color: var(--text-muted); font-size: 0.9em; border-top: 1px solid var(--border-color); }";
  html += "</style></head><body>";
  html += getNavigation("admin");
  html += "<div class='container'>";
  html += "<h1>Stored Preferences</h1>";

  // Open preferences in read-only mode
  preferences.begin("mmdvm", true);

  // Define categories and their keys
  struct Category {
    const char* name;
    const char** keys;
    int keyCount;
  };

  const char* dmrKeys[] = {
    "dmr_callsign", "dmr_id", "dmr_server", "dmr_password", "dmr_essid",
    "dmr_rx_freq", "dmr_tx_freq", "dmr_power", "dmr_cc",
    "dmr_lat", "dmr_lon", "dmr_height", "dmr_location",
    "dmr_desc", "dmr_url"
  };

  const char* wifiKeys[] = {
    "wifi0_label", "wifi0_ssid", "wifi0_pass",
    "wifi1_label", "wifi1_ssid", "wifi1_pass",
    "wifi2_label", "wifi2_ssid", "wifi2_pass",
    "wifi3_label", "wifi3_ssid", "wifi3_pass",
    "wifi4_label", "wifi4_ssid", "wifi4_pass"
  };

  const char* systemKeys[] = {
    "hostname", "verbose_log", "debug_serial", "debug_mmdvm", "debug_network",
    "debug_dmr", "debug_password", "enable_oled", "oled_autoblank", "oled_blank_to",
    "ntp_tz_offset", "ntp_dst_offset", "modem_type"
  };

  const char* modeKeys[] = {
    "mode_dmr", "mode_dstar", "mode_ysf", "mode_p25", "mode_nxdn", "mode_pocsag"
  };

  const char* webKeys[] = {
    "web_username", "web_password"
  };

  const char* mqttKeys[] = {
    "mqtt_enabled", "mqtt_broker", "mqtt_port", "mqtt_user", "mqtt_pass",
    "mqtt_client", "mqtt_prefix", "mqtt_interval"
  };

  Category categories[] = {
    {"DMR Configuration", dmrKeys, sizeof(dmrKeys) / sizeof(dmrKeys[0])},
    {"WiFi Networks", wifiKeys, sizeof(wifiKeys) / sizeof(wifiKeys[0])},
    {"System Settings", systemKeys, sizeof(systemKeys) / sizeof(systemKeys[0])},
    {"Mode Configuration", modeKeys, sizeof(modeKeys) / sizeof(modeKeys[0])},
    {"Web Interface", webKeys, sizeof(webKeys) / sizeof(webKeys[0])},
    {"MQTT Configuration", mqttKeys, sizeof(mqttKeys) / sizeof(mqttKeys[0])}
  };

  int categoryCount = sizeof(categories) / sizeof(categories[0]);
  int totalFoundKeys = 0;

  // Loop through each category
  for (int catIdx = 0; catIdx < categoryCount; catIdx++) {
    Category& category = categories[catIdx];
    String categoryHtml = "";
    int categoryFoundKeys = 0;

    // Build table for this category
    categoryHtml += "<table class='pref-table'>";
    categoryHtml += "<thead><tr><th>Key</th><th>Value</th><th>Type</th><th>Size (bytes)</th></tr></thead><tbody>";

    for (int i = 0; i < category.keyCount; i++) {
      String keyName = String(category.keys[i]);

      if (preferences.isKey(keyName.c_str())) {
        categoryFoundKeys++;
        totalFoundKeys++;

        // Try to determine the type by attempting different reads
        String value = "";
        String type = "";
        size_t keySize = 0;

        // Check the expected type based on key name and only try that type
        if (keyName == "dmr_id" || keyName == "dmr_rx_freq" || keyName == "dmr_tx_freq") {
          // Known UInt32 keys
          uint32_t uintVal = preferences.getUInt(keyName.c_str(), 0xFFFFFFFF);
          if (uintVal != 0xFFFFFFFF) {
            value = String(uintVal);
            if (keyName.indexOf("freq") >= 0 && uintVal > 100000) {
              value += " Hz";
            }
            type = "UInt32";
            keySize = 4;
          }
        }
        else if (keyName == "dmr_essid" || keyName == "dmr_power" || keyName == "dmr_cc") {
          // Known UChar keys
          uint8_t ucharVal = preferences.getUChar(keyName.c_str(), 255);
          if (ucharVal != 255) {
            value = String(ucharVal);
            type = "UChar";
            keySize = 1;
          }
        }
        else if (keyName == "dmr_height") {
          // Known Int32 keys
          int32_t intVal = preferences.getInt(keyName.c_str(), -999999);
          if (intVal != -999999) {
            value = String(intVal);
            if (keyName.indexOf("height") >= 0) {
              value += " meters";
            }
            type = "Int32";
            keySize = 4;
          }
        }
        else if (keyName == "ntp_tz_offset" || keyName == "ntp_dst_offset") {
          // Known Long keys for timezone
          long longVal = preferences.getLong(keyName.c_str(), -999999);
          if (longVal != -999999) {
            value = String(longVal);
            if (keyName == "ntp_tz_offset") {
              value += " sec (" + String(longVal / 3600.0, 1) + " hours)";
            } else {
              value += " sec (" + String(longVal / 3600.0, 1) + " hours DST)";
            }
            type = "Long";
            keySize = 4;
          }
        }
        else if (keyName == "oled_blank_to") {
          // OLED blank timeout (ULong, stored in milliseconds)
          unsigned long ulongVal = preferences.getULong(keyName.c_str(), 0);
          if (ulongVal > 0 || preferences.isKey(keyName.c_str())) {
            value = String(ulongVal);
            value += " ms (" + String(ulongVal / 1000) + " sec)";
            type = "ULong";
            keySize = 4;
          }
        }
        else if (keyName == "dmr_lat" || keyName == "dmr_lon") {
          // Known Float keys
          float floatVal = preferences.getFloat(keyName.c_str(), -999.999);
          if (floatVal != -999.999) {
            value = String(floatVal, 6);
            type = "Float";
            keySize = 4;
          }
        }
        else if (keyName == "mqtt_port") {
          // MQTT port (UShort)
          uint16_t ushortVal = preferences.getUShort(keyName.c_str(), 0);
          if (ushortVal > 0 || preferences.isKey(keyName.c_str())) {
            value = String(ushortVal);
            type = "UShort";
            keySize = 2;
          }
        }
        else if (keyName == "mqtt_interval") {
          // MQTT publish interval (UInt32, stored in milliseconds)
          uint32_t uintVal = preferences.getUInt(keyName.c_str(), 0);
          if (uintVal > 0 || preferences.isKey(keyName.c_str())) {
            value = String(uintVal);
            value += " ms (" + String(uintVal / 1000) + " sec)";
            type = "UInt32";
            keySize = 4;
          }
        }
        else if (keyName == "verbose_log" || keyName == "enable_oled" || keyName == "oled_autoblank" ||
                 keyName == "mqtt_enabled" || keyName.startsWith("mode_") || keyName.startsWith("debug_")) {
          // Known Bool keys (verbose_log, enable_oled, oled_autoblank, mqtt_enabled, mode_*, debug_*)
          bool boolVal = preferences.getBool(keyName.c_str(), false);
          value = String(boolVal ? "true" : "false");
          type = "Bool";
          keySize = 1;
        }
        else {
          // Assume string for all other keys
          size_t strLen = preferences.getBytesLength(keyName.c_str());
          if (strLen > 0) {
            String strValue = preferences.getString(keyName.c_str(), "");
            // Check if this is a password field
            if (keyName.equals("dmr_password") || keyName.equals("alt_password") ||
                keyName.equals("web_password") || keyName.indexOf("password") >= 0 ||
                keyName.endsWith("_pass")) {
              if (strValue.length() > 0) {
                String maskedPassword = "";
                for (int j = 0; j < strValue.length(); j++) {
                  maskedPassword += "*";
                }
                // Create toggleable password display
                String passwordId = "pwd" + keyName;
                passwordId.replace("_", "");  // Remove underscores for valid ID
                value = "<div class='password-container'>";
                value += "<span id='" + passwordId + "masked' class='password-hidden'>" + maskedPassword + "</span>";
                value += "<span id='" + passwordId + "real' style='display:none;'>" + strValue + "</span>";
                value += "<span class='password-toggle' onclick='togglePassword(\"" + passwordId + "\")' title='Show/Hide Password'>&nbsp;&#x1F441;</span>";
                value += "</div>";
                type = "String (password)";
              } else {
                value = "[EMPTY PASSWORD]";
                type = "String (empty)";
              }
            } else {
              if (strValue.length() > 0) {
                value = strValue;
                type = "String";
              } else {
                value = "[EMPTY STRING]";
                type = "String (empty)";
              }
            }
            keySize = strLen;
          } else {
            // Check if this key exists but as a different type or is completely missing
            String testValue = preferences.getString(keyName.c_str(), "__NOT_FOUND__");
            if (testValue != "__NOT_FOUND__") {
              // Check if this is a password field
              if (keyName.equals("dmr_password") || keyName.equals("alt_password") ||
                  keyName.equals("web_password") || keyName.indexOf("password") >= 0 ||
                  keyName.endsWith("_pass")) {
                if (testValue.length() > 0) {
                  String maskedPassword = "";
                  for (int j = 0; j < testValue.length(); j++) {
                    maskedPassword += "*";
                  }
                  // Create toggleable password display
                  String passwordId = "pwd" + keyName;
                  passwordId.replace("_", "");  // Remove underscores for valid ID
                  value = "<div class='password-container'>";
                  value += "<span id='" + passwordId + "masked' class='password-hidden'>" + maskedPassword + "</span>";
                  value += "<span id='" + passwordId + "real' style='display:none;'>" + testValue + "</span>";
                  value += "<span class='password-toggle' onclick='togglePassword(\"" + passwordId + "\")' title='Show/Hide Password'>&nbsp;&#x1F441;</span>";
                  value += "</div>";
                  type = "String (password)";
                } else {
                  value = "[EMPTY PASSWORD]";
                  type = "String (empty)";
                }
              } else {
                value = testValue.length() > 0 ? testValue : "[EMPTY STRING]";
                type = testValue.length() > 0 ? "String" : "String (empty)";
              }
              keySize = testValue.length() + 1; // +1 for null terminator
            }
          }
        }

        // Show the preference if we found any value or type information
        if (value != "" || type != "") {
          categoryHtml += "<tr><td class='pref-key'>" + keyName + "</td><td class='pref-value'>" + (value != "" ? value : "[NOT STORED]") + "</td><td class='pref-type'>" + (type != "" ? type : "Unknown") + "</td><td class='pref-type'>" + String(keySize) + "</td></tr>";
        }
      }
    }

    categoryHtml += "</tbody></table>";

    // Only show category if it has keys
    if (categoryFoundKeys > 0) {
      html += "<div class='pref-category'>";
      html += "<div class='pref-category-header' onclick='toggleCategory(\"cat" + String(catIdx) + "\")'>";
      html += "<span>" + String(category.name) + "</span>";
      html += "<span class='pref-category-toggle' id='cat" + String(catIdx) + "toggle'>&#9660;</span>";
      html += "</div>";
      html += "<div class='pref-category-content' id='cat" + String(catIdx) + "'>";
      html += categoryHtml;
      html += "<div class='category-summary'>" + String(categoryFoundKeys) + " preference" + (categoryFoundKeys != 1 ? "s" : "") + " found</div>";
      html += "</div>";
      html += "</div>";
    }
  }

  if (totalFoundKeys == 0) {
    html += "<p style='text-align: center; color: #6c757d; font-style: italic;'>No preferences found in 'mmdvm' namespace</p>";
  } else {
    html += "<p style='text-align: center; color: var(--text-muted); font-style: italic; margin-top: 20px;'>";
    html += "Total: " + String(totalFoundKeys) + " stored preferences across " + String(categoryCount) + " categories";
    html += "</p>";
  }

  preferences.end();
  // Add JavaScript for password toggle and category collapse functionality
  html += "<script>";
  html += "function togglePassword(passwordId) {";
  html += "  var masked = document.getElementById(passwordId + 'masked');";
  html += "  var real = document.getElementById(passwordId + 'real');";
  html += "  var toggle = masked.parentElement.querySelector('.password-toggle');";
  html += "  if (masked.style.display === 'none') {";
  html += "    masked.style.display = 'inline';";
  html += "    real.style.display = 'none';";
  html += "    toggle.innerHTML = '&#128065;';";
  html += "    toggle.title = 'Show Password';";
  html += "  } else {";
  html += "    masked.style.display = 'none';";
  html += "    real.style.display = 'inline';";
  html += "    toggle.innerHTML = '&#128064;';";
  html += "    toggle.title = 'Hide Password';";
  html += "  }";
  html += "}";
  html += "function toggleCategory(categoryId) {";
  html += "  var content = document.getElementById(categoryId);";
  html += "  var toggle = document.getElementById(categoryId + 'toggle');";
  html += "  if (content.classList.contains('collapsed')) {";
  html += "    content.classList.remove('collapsed');";
  html += "    toggle.classList.remove('collapsed');";
  html += "    toggle.innerHTML = '&#9660;';";
  html += "  } else {";
  html += "    content.classList.add('collapsed');";
  html += "    toggle.classList.add('collapsed');";
  html += "    toggle.innerHTML = '&#9660;';";
  html += "  }";
  html += "}";
  html += "</script>";
  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html; charset=UTF-8", html);
  logSerial("Preferences display requested by user");
}

#endif // SYSTEM_HANDLERS_H
