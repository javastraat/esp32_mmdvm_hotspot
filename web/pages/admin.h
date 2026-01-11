/*
 * admin.h - Admin Page and Functions for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_PAGES_ADMIN_H
#define WEB_PAGES_ADMIN_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <Update.h>
#include <PubSubClient.h>

// External variables and functions
extern WebServer server;
extern PubSubClient mqttClient;
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
extern String web_password;
extern String serialLog[];
extern int serialLogIndex;
extern Preferences preferences;
extern String firmwareVersion;
extern String modemFirmwareVersion;
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
extern bool mqttConnected;
extern unsigned long lastMqttPublish;
extern String lastMqttError;
extern unsigned long mqttConnectAttempts;
extern unsigned long lastMqttConnectAttempt;
extern void logSerial(String message);
extern void saveConfig();
extern String getCommonCSS();
extern String getNavigation(String activePage);
extern String getFooter();


#include "../../include/handlers/admin/system_handlers.h"
#include "../../include/handlers/admin/settings_handlers.h"
#include "../../include/handlers/admin/security_handlers.h"
#include "../../include/handlers/admin/network_handlers.h"
#include "../../include/handlers/admin/maintenance_handlers.h"
// Constants already defined in config.h
#define SERIAL_LOG_SIZE 50

// ===== ADMIN PAGE HANDLERS =====


// ===== ADMIN PAGE HANDLERS =====

void handleAdmin() {
  if (!checkAuthentication()) return;

  // Get submenu parameter from URL
  String submenu = "";
  if (server.hasArg("submenu")) {
    submenu = server.arg("submenu");
  }

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>" + dmr_callsign + " - ESP32 MMDVM Hotspot</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".admin-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 20px 0; }";
  html += ".metric { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #eee; }";
  html += ".metric:last-child { border-bottom: none; }";
  html += ".metric-label { font-weight: bold; color: #555; }";
  html += ".metric-value { color: #333; }";
  html += ".uptime { color: #007bff; font-weight: bold; }";
  html += ".btn { display: inline-block; padding: 12px 24px; margin: 10px 5px; border: none; border-radius: 6px; cursor: pointer; text-decoration: none; font-size: 14px; font-weight: bold; text-align: center; transition: background-color 0.3s; }";
  html += ".btn-primary { background: #007bff; color: white; }";
  html += ".btn-primary:hover { background: #0056b3; }";
  html += ".btn-success { background: #28a745; color: white; }";
  html += ".btn-success:hover { background: #218838; }";
  html += ".btn-warning { background: #ffc107; color: black; }";
  html += ".btn-warning:hover { background: #e0a800; }";
  html += ".btn-danger { background: #dc3545; color: white; }";
  html += ".btn-danger:hover { background: #c82333; }";
  html += ".btn-info { background: #17a2b8; color: white; }";
  html += ".btn-info:hover { background: #138496; }";
  html += ".action-buttons { text-align: center; margin: 15px 0; }";
  html += ".action-buttons-vertical { text-align: center; margin: 15px 0; }";
  html += ".action-buttons-vertical .btn { display: block; margin: 8px auto; width: 80%; }";
  html += ".menu-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; margin: 20px 0; }";
  html += ".menu-card { background: var(--container-bg); border: 1px solid var(--border-color); border-radius: 8px; padding: 30px; text-align: center; transition: transform 0.2s, box-shadow 0.2s; cursor: pointer; }";
  html += ".menu-card:hover { transform: translateY(-5px); box-shadow: 0 5px 15px rgba(0,0,0,0.2); }";
  html += ".menu-card h3 { margin: 0 0 10px 0; }";
  html += ".menu-card p { color: #666; font-size: 0.9em; }";
  html += "</style></head><body>";
  html += getNavigation("admin");
  html += "<div class='container'>";
  
  // Show submenu selector or submenu content
  if (submenu == "") {
    // Main menu - show category buttons
    html += "<h1>System Administration</h1>";
    html += "<p>Select a category to manage:</p>";
    html += "<div class='menu-grid'>";
    
    html += "<a href='/admin?submenu=system' style='text-decoration: none; color: inherit;'>";
    html += "<div class='menu-card'>";
    html += "<h3>System</h3>";
    html += "<p>System & modem information, control functions</p>";
    html += "</div></a>";
    
    html += "<a href='/admin?submenu=settings' style='text-decoration: none; color: inherit;'>";
    html += "<div class='menu-card'>";
    html += "<h3>Settings</h3>";
    html += "<p>Hostname, debug, OLED, timezone configuration</p>";
    html += "</div></a>";
    
    html += "<a href='/admin?submenu=security' style='text-decoration: none; color: inherit;'>";
    html += "<div class='menu-card'>";
    html += "<h3>Security</h3>";
    html += "<p>Web username and password management</p>";
    html += "</div></a>";
    
    html += "<a href='/admin?submenu=network' style='text-decoration: none; color: inherit;'>";
    html += "<div class='menu-card'>";
    html += "<h3>Network</h3>";
    html += "<p>MQTT configuration and settings</p>";
    html += "</div></a>";
    
    html += "<a href='/admin?submenu=maintenance' style='text-decoration: none; color: inherit;'>";
    html += "<div class='menu-card'>";
    html += "<h3>Maintenance</h3>";
    html += "<p>Configuration, firmware updates, modem flashing</p>";
    html += "</div></a>";
    
    html += "</div>";
  } else {
    // Submenu view - show back button and title
    html += "<div style='margin-bottom: 20px;'>";
    html += "<a href='/admin' class='btn btn-primary'>Back to Admin Menu</a>";
    html += "</div>";
    
    if (submenu == "system") {
      html += "<h1>System</h1>";
    } else if (submenu == "settings") {
      html += "<h1>Settings</h1>";
    } else if (submenu == "security") {
      html += "<h1>Security</h1>";
    } else if (submenu == "network") {
      html += "<h1>Network</h1>";
    } else if (submenu == "maintenance") {
      html += "<h1>Maintenance</h1>";
    }
    
    html += "<div class='admin-grid'>";
    
    // SYSTEM SUBMENU
    if (submenu == "system") {
      // System Information Card
      html += "<div class='card'>";
      html += "<h3>System Information</h3>";

      // Uptime calculation
      unsigned long uptimeSeconds = millis() / 1000;
      unsigned long days = uptimeSeconds / 86400;
      unsigned long hours = (uptimeSeconds % 86400) / 3600;
      unsigned long minutes = (uptimeSeconds % 3600) / 60;
      unsigned long seconds = uptimeSeconds % 60;
      String uptimeStr = "";
      if (days > 0) uptimeStr += String(days) + "d ";
      if (hours > 0 || days > 0) uptimeStr += String(hours) + "h ";
      uptimeStr += String(minutes) + "m " + String(seconds) + "s";

      html += "<div class='metric'><span class='metric-label'>Uptime:</span><span class='metric-value uptime'>" + uptimeStr + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Chip Model:</span><span class='metric-value'>" + String(ESP.getChipModel()) + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Chip Revision:</span><span class='metric-value'>" + String(ESP.getChipRevision()) + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>CPU Cores:</span><span class='metric-value'>" + String(ESP.getChipCores()) + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>CPU Frequency:</span><span class='metric-value'>" + String(ESP.getCpuFreqMHz()) + " MHz</span></div>";
      html += "<div class='metric'><span class='metric-label'>Free Heap:</span><span class='metric-value'>" + String(ESP.getFreeHeap()/1024.0, 1) + " KB (" + String(ESP.getFreeHeap()*100/ESP.getHeapSize()) + "%)</span></div>";
      html += "<div class='metric'><span class='metric-label'>Min Free Heap:</span><span class='metric-value'>" + String(ESP.getMinFreeHeap()/1024.0, 1) + " KB</span></div>";
      html += "<div class='metric'><span class='metric-label'>Heap Size:</span><span class='metric-value'>" + String(ESP.getHeapSize()/1024.0, 1) + " KB</span></div>";

      // PSRAM info (if available)
      if (ESP.getPsramSize() > 0) {
        html += "<div class='metric'><span class='metric-label'>PSRAM Size:</span><span class='metric-value'>" + String(ESP.getPsramSize()/1024/1024) + " MB</span></div>";
        html += "<div class='metric'><span class='metric-label'>Free PSRAM:</span><span class='metric-value'>" + String(ESP.getFreePsram()/1024.0, 1) + " KB</span></div>";
      }

      html += "<div class='metric'><span class='metric-label'>Flash Size:</span><span class='metric-value'>" + String(ESP.getFlashChipSize()/1024/1024) + " MB</span></div>";
      html += "<div class='metric'><span class='metric-label'>Flash Speed:</span><span class='metric-value'>" + String(ESP.getFlashChipSpeed()/1000000) + " MHz</span></div>";
      html += "<div class='metric'><span class='metric-label'>Sketch Size:</span><span class='metric-value'>" + String(ESP.getSketchSize()/1024.0, 1) + " KB</span></div>";
      html += "<div class='metric'><span class='metric-label'>Free Sketch Space:</span><span class='metric-value'>" + String(ESP.getFreeSketchSpace()/1024.0, 1) + " KB</span></div>";
      html += "<div class='metric'><span class='metric-label'>SDK Version:</span><span class='metric-value'>" + String(ESP.getSdkVersion()) + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Firmware Version:</span><span class='metric-value'>" + String(FIRMWARE_VERSION) + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Build Date:</span><span class='metric-value'>" + String(__DATE__) + " " + String(__TIME__) + "</span></div>";
      html += "</div>";

      // Modem Information Card - Parse the firmware version string
      html += "<div class='card'>";
      html += "<h3>Modem Information</h3>";

      // Parse modem firmware version string
      // Example: "MMDVM_HS_Hat-v1.5.2 20201108 14.7456MHz ADF7021 FW by CA6JAU GitID #89daa20"
      String hardware = "Unknown";
      String version = "Unknown";
      String buildDate = "Unknown";
      String crystal = "Unknown";
      String transceiver = "Unknown";
      String author = "Unknown";
      String gitId = "Unknown";

      if (modemFirmwareVersion != "Unknown" && modemFirmwareVersion.length() > 0) {
        String fwStr = modemFirmwareVersion;

        // Extract hardware (before "-v" or first space)
        int vPos = fwStr.indexOf("-v");
        if (vPos > 0) {
          hardware = fwStr.substring(0, vPos);
          fwStr = fwStr.substring(vPos + 2); // Skip "-v"
        } else {
          int spacePos = fwStr.indexOf(' ');
          if (spacePos > 0) {
            hardware = fwStr.substring(0, spacePos);
            fwStr = fwStr.substring(spacePos + 1);
          }
        }

        // Extract version (digits and dots until space)
        int spacePos = fwStr.indexOf(' ');
        if (spacePos > 0) {
          version = fwStr.substring(0, spacePos);
          fwStr = fwStr.substring(spacePos + 1);
        }

        // Extract build date (8 digits, may have suffix like _WPSD)
        spacePos = fwStr.indexOf(' ');
        if (spacePos > 0) {
          String dateStr = fwStr.substring(0, spacePos);

          // Check if first 8 characters are digits (YYYYMMDD)
          if (dateStr.length() >= 8) {
            bool isValidDate = true;
            for (int i = 0; i < 8; i++) {
              if (!isdigit(dateStr.charAt(i))) {
                isValidDate = false;
                break;
              }
            }

            if (isValidDate) {
                      // Format YYYYMMDD to YYYY-MM-DD (ignore any suffix like _WPSD)
              //buildDate = dateStr.substring(0, 4) + "-" + dateStr.substring(4, 6) + "-" + dateStr.substring(6, 8);
 
              // Format YYYYMMDD to DD-MM-YYYY (ignore any suffix like _WPSD)
              buildDate = dateStr.substring(6, 8) + "-" + dateStr.substring(4, 6) + "-" + dateStr.substring(0, 4);
            }
          }
          fwStr = fwStr.substring(spacePos + 1);
        }

        // Extract crystal frequency (number followed by MHz)
        int mhzPos = fwStr.indexOf("MHz");
        if (mhzPos > 0) {
          int startPos = 0;
          for (int i = mhzPos - 1; i >= 0; i--) {
            if (fwStr.charAt(i) == ' ') {
              startPos = i + 1;
              break;
            }
          }
          crystal = fwStr.substring(startPos, mhzPos + 3);
          fwStr = fwStr.substring(mhzPos + 3);
        }

        // Extract transceiver (word after MHz, before " FW")
        fwStr.trim();
        int fwPos = fwStr.indexOf(" FW");
        if (fwPos > 0) {
          // Get the first word (transceiver name)
          spacePos = fwStr.indexOf(' ');
          if (spacePos > 0) {
            transceiver = fwStr.substring(0, spacePos);
          } else {
            // No space found, use everything before " FW"
            transceiver = fwStr.substring(0, fwPos);
          }
          fwStr = fwStr.substring(fwPos + 3); // Skip " FW"
        }

        // Extract author (after "by " before " GitID")
        int byPos = fwStr.indexOf("by ");
        int gitPos = fwStr.indexOf(" GitID");
        if (byPos >= 0 && gitPos > byPos) {
          author = fwStr.substring(byPos + 3, gitPos);
          author.trim();
        }

        // Extract Git ID (after "GitID ")
        gitPos = fwStr.indexOf("GitID ");
        if (gitPos >= 0) {
          gitId = fwStr.substring(gitPos + 6);
          gitId.trim();
        }
      }

      html += "<div class='metric'><span class='metric-label'>Hardware:</span><span class='metric-value'>" + hardware + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Firmware Version:</span><span class='metric-value'>" + version + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Build Date:</span><span class='metric-value'>" + buildDate + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Crystal:</span><span class='metric-value'>" + crystal + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Transceiver:</span><span class='metric-value'>" + transceiver + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Author:</span><span class='metric-value'>" + author + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Git ID:</span><span class='metric-value'>" + gitId + "</span></div>";
      html += "</div>";

      // System Control Card
      html += "<div class='card'>";
      html += "<h3>System Control</h3>";
      html += "<p>Control basic system functions:</p>";
      html += "<div class='action-buttons-vertical'>";
      html += "<a href='javascript:void(0)' onclick='rebootSystem()' class='btn btn-warning'>Reboot System</a>";
      html += "<a href='javascript:void(0)' onclick='restartServices()' class='btn btn-primary'>Restart Services</a>";
      html += "</div>";
      html += "</div>";
    }
    
    // SETTINGS SUBMENU
    if (submenu == "settings") {
      // Hostname Configuration Card
      html += "<div class='card'>";
      html += "<h3>Hostname Configuration</h3>";
      html += "<p>Current hostname: <strong>" + device_hostname + "</strong></p>";
      html += "<p style='font-size:0.9em;color:var(--text-color);'>Access via: http://" + device_hostname + ".local</p>";
      html += "<form id='hostname-form' onsubmit='saveHostname(event)'>";
      html += "<input type='text' id='hostname-input' value='" + device_hostname + "' placeholder='e.g., mmdvm-hotspot' style='width:100%;padding:10px;margin:10px 0;border:1px solid var(--border-color);border-radius:4px;box-sizing:border-box;background:var(--container-bg);color:var(--text-color);' pattern='[a-zA-Z0-9-]{1,32}' required>";
      html += "<button type='submit' class='btn btn-success' style='width:100%;'>Save Hostname</button>";
      html += "</form>";
      html += "</div>";

      // Debug Settings Card (includes verbose logging)
      html += "<div class='card'>";
      html += "<h3>Debug Settings</h3>";
      html += "<p>Control debug output visibility</p>";
      html += "<p style='font-size:0.9em;color:var(--text-color);'>Enable specific debug categories to troubleshoot issues.</p>";
      html += "<form id='debug-form' onsubmit='saveDebugSettings(event)'>";
      html += "<div style='display:flex;flex-direction:column;gap:8px;margin:10px 0;'>";

      // DEBUG_SERIAL
      html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;'>";
      html += "<input type='checkbox' id='debug-serial' " + String(debug_serial ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
      html += "<span><strong>Serial Output</strong> - General serial debug messages</span>";
      html += "</label>";
 
      // Verbose Logging (show keepalive in web interface)
      html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;'>";
      html += "<input type='checkbox' id='verbose-logging' " + String(verbose_logging ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
      html += "<span><strong>Verbose Logging</strong> - Show keepalive in Serial Monitor web page</span>";
      html += "</label>";
 
      // DEBUG_MMDVM
      html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;'>";
      html += "<input type='checkbox' id='debug-mmdvm' " + String(debug_mmdvm ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
      html += "<span><strong>MMDVM Protocol</strong> - TX frame debug (verbose)</span>";
      html += "</label>";
      
      // DEBUG_NETWORK
      html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;'>";
      html += "<input type='checkbox' id='debug-network' " + String(debug_network ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
      html += "<span><strong>Network Debug</strong> - Keepalive messages (verbose)</span>";
      html += "</label>";
      
      // DEBUG_DMR
      html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;'>";
      html += "<input type='checkbox' id='debug-dmr' " + String(debug_dmr ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
      html += "<span><strong>DMR Protocol</strong> - DMR packet details (reserved)</span>";
      html += "</label>";
      
      // DEBUG_PASSWORD
      html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;'>";
      html += "<input type='checkbox' id='debug-password' " + String(debug_password ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
      html += "<span><strong>Password Debug</strong> - Show password length/last4 chars</span>";
      html += "</label>";
      
      html += "</div>";
      html += "<button type='submit' class='btn btn-success' style='width:100%;margin-top:10px;'>Save Debug Settings</button>";
      html += "</form>";
      html += "</div>";

      // OLED Display Settings Card
      html += "<div class='card'>";
      html += "<h3>OLED Display Settings</h3>";
      html += "<p>Enable or disable OLED display (SSD1306)</p>";
      html += "<p>Current status: <strong>" + String(enable_oled ? "Enabled" : "Disabled") + "</strong></p>";
      html += "<p style='font-size:0.9em;color:var(--text-color);'>Toggle OLED display without recompiling. Changes take effect after reboot.</p>";
      html += "<form id='oled-form' onsubmit='saveOLEDSettings(event)'>";
      html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;'>";
      html += "<input type='checkbox' id='enable-oled' " + String(enable_oled ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
      html += "<span><strong>Enable OLED Display</strong> - Show status on 128x64 OLED screen</span>";
      html += "</label>";

      // Auto-blanking settings
      html += "<div style='margin-top:20px;padding-top:15px;border-top:1px solid var(--border-color);'>";
      html += "<h4 style='margin:0 0 10px 0;'>Auto-Blanking Settings</h4>";
      html += "<p style='font-size:0.9em;color:var(--text-color);margin-bottom:15px;'>Automatically turn off display after inactivity to prevent burn-in</p>";

      html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;margin-bottom:15px;'>";
      html += "<input type='checkbox' id='auto-blank-enable' " + String(oledAutoBlankEnabled ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
      html += "<span><strong>Enable Auto-Blanking</strong> - Screen turns off after inactivity</span>";
      html += "</label>";

      html += "<div>";
      html += "<label style='display:block;margin-bottom:5px;'><strong>Blank Timeout:</strong></label>";
      html += "<select id='blank-timeout' style='width:100%;padding:10px;border:1px solid var(--border-color);border-radius:4px;background:var(--container-bg);color:var(--text-color);'>";
      html += "<option value='0' " + String(oledBlankTimeout == 0 ? "selected" : "") + ">Never (disabled)</option>";
      html += "<option value='30000' " + String(oledBlankTimeout == 30000 ? "selected" : "") + ">30 seconds</option>";
      html += "<option value='60000' " + String(oledBlankTimeout == 60000 ? "selected" : "") + ">1 minute</option>";
      html += "<option value='120000' " + String(oledBlankTimeout == 120000 ? "selected" : "") + ">2 minutes</option>";
      html += "<option value='300000' " + String(oledBlankTimeout == 300000 ? "selected" : "") + ">5 minutes</option>";
      html += "<option value='600000' " + String(oledBlankTimeout == 600000 ? "selected" : "") + ">10 minutes</option>";
      html += "</select>";
      html += "<p style='font-size:0.85em;color:var(--text-color);margin-top:5px;'>Screen will wake up automatically when DMR activity is detected</p>";
      html += "</div>";
      html += "</div>";

      html += "<button type='submit' class='btn btn-success' style='width:100%;margin-top:15px;'>Save OLED Settings</button>";
      html += "</form>";
      html += "</div>";

      // NTP Timezone Configuration Card
      html += "<div class='card'>";
      html += "<h3>NTP Timezone Configuration</h3>";
      html += "<p>Configure timezone for timestamps</p>";
      html += "<p>Current timezone offset: <strong>" + String(ntp_timezone_offset / 3600.0, 1) + " hours</strong></p>";
      html += "<p>Current DST offset: <strong>" + String(ntp_daylight_offset / 3600.0, 1) + " hours</strong></p>";
      html += "<form id='timezone-form' onsubmit='saveTimezone(event)'>";
      html += "<label>Timezone Offset (hours from UTC):</label>";
      html += "<select id='timezone-select' style='width:100%;padding:10px;margin:10px 0;border:1px solid var(--border-color);border-radius:4px;background:var(--container-bg);color:var(--text-color);'>";
      html += "<option value='-43200' " + String(ntp_timezone_offset == -43200 ? "selected" : "") + ">UTC-12</option>";
      html += "<option value='-39600' " + String(ntp_timezone_offset == -39600 ? "selected" : "") + ">UTC-11</option>";
      html += "<option value='-36000' " + String(ntp_timezone_offset == -36000 ? "selected" : "") + ">UTC-10 (Hawaii)</option>";
      html += "<option value='-32400' " + String(ntp_timezone_offset == -32400 ? "selected" : "") + ">UTC-9 (Alaska)</option>";
      html += "<option value='-28800' " + String(ntp_timezone_offset == -28800 ? "selected" : "") + ">UTC-8 (PST)</option>";
      html += "<option value='-25200' " + String(ntp_timezone_offset == -25200 ? "selected" : "") + ">UTC-7 (MST)</option>";
      html += "<option value='-21600' " + String(ntp_timezone_offset == -21600 ? "selected" : "") + ">UTC-6 (CST)</option>";
      html += "<option value='-18000' " + String(ntp_timezone_offset == -18000 ? "selected" : "") + ">UTC-5 (EST)</option>";
      html += "<option value='-14400' " + String(ntp_timezone_offset == -14400 ? "selected" : "") + ">UTC-4 (AST)</option>";
      html += "<option value='-10800' " + String(ntp_timezone_offset == -10800 ? "selected" : "") + ">UTC-3</option>";
      html += "<option value='-7200' " + String(ntp_timezone_offset == -7200 ? "selected" : "") + ">UTC-2</option>";
      html += "<option value='-3600' " + String(ntp_timezone_offset == -3600 ? "selected" : "") + ">UTC-1</option>";
      html += "<option value='0' " + String(ntp_timezone_offset == 0 ? "selected" : "") + ">UTC+0 (GMT)</option>";
      html += "<option value='3600' " + String(ntp_timezone_offset == 3600 ? "selected" : "") + ">UTC+1 (CET)</option>";
      html += "<option value='7200' " + String(ntp_timezone_offset == 7200 ? "selected" : "") + ">UTC+2 (EET)</option>";
      html += "<option value='10800' " + String(ntp_timezone_offset == 10800 ? "selected" : "") + ">UTC+3</option>";
      html += "<option value='14400' " + String(ntp_timezone_offset == 14400 ? "selected" : "") + ">UTC+4</option>";
      html += "<option value='18000' " + String(ntp_timezone_offset == 18000 ? "selected" : "") + ">UTC+5</option>";
      html += "<option value='21600' " + String(ntp_timezone_offset == 21600 ? "selected" : "") + ">UTC+6</option>";
      html += "<option value='25200' " + String(ntp_timezone_offset == 25200 ? "selected" : "") + ">UTC+7</option>";
      html += "<option value='28800' " + String(ntp_timezone_offset == 28800 ? "selected" : "") + ">UTC+8</option>";
      html += "<option value='32400' " + String(ntp_timezone_offset == 32400 ? "selected" : "") + ">UTC+9</option>";
      html += "<option value='36000' " + String(ntp_timezone_offset == 36000 ? "selected" : "") + ">UTC+10 (AEST)</option>";
      html += "<option value='39600' " + String(ntp_timezone_offset == 39600 ? "selected" : "") + ">UTC+11</option>";
      html += "<option value='43200' " + String(ntp_timezone_offset == 43200 ? "selected" : "") + ">UTC+12</option>";
      html += "</select>";
      html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;margin:15px 0;'>";
      html += "<input type='checkbox' id='dst-checkbox' " + String(ntp_daylight_offset == 3600 ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
      html += "<span>Enable Daylight Saving Time (+1 hour)</span>";
      html += "</label>";
      html += "<button type='submit' class='btn btn-success' style='width:100%;'>Save Timezone</button>";
      html += "</form>";
      html += "</div>";
    }
    
    // SECURITY SUBMENU
    if (submenu == "security") {
      // Web Username Card
      html += "<div class='card'>";
      html += "<h3>Web Username</h3>";
      html += "<p>Manage web interface username</p>";
      html += "<div style='background:var(--info-bg);padding:10px;border-radius:4px;margin-bottom:15px;'>";
      html += "<div style='display:flex;justify-content:space-between;align-items:center;'>";
      html += "<span><strong>Current Username:</strong></span>";
      html += "<span id='current-username-display'>" + web_username + "</span>";
      html += "</div>";
      html += "</div>";

      html += "<form id='username-form' onsubmit='saveUsername(event)'>";
      html += "<label>New Username:</label>";
      html += "<input type='text' id='new-username' placeholder='Enter new username' value='" + web_username + "' required style='width:100%;padding:8px;margin:5px 0;box-sizing:border-box;'>";
      html += "<p style='font-size:0.85em;color:#666;margin:5px 0;'>Username must be at least 3 characters</p>";
      html += "<button type='submit' class='btn btn-info' style='width:100%;margin-top:10px;'>Update Username</button>";
      html += "</form>";
      html += "</div>";

      // Web Password Card
      html += "<div class='card'>";
      html += "<h3>Web Password</h3>";
      html += "<p>Manage web interface password</p>";
      html += "<div style='background:var(--info-bg);padding:10px;border-radius:4px;margin-bottom:15px;'>";
      html += "<div style='display:flex;justify-content:space-between;align-items:center;'>";
      html += "<span><strong>Current Password:</strong></span>";
      html += "<div style='display:flex;align-items:center;gap:8px;'>";
      html += "<span id='current-password-display' style='font-family:monospace;'>********</span>";
      html += "<span onclick='toggleCurrentPassword()' style='cursor:pointer;font-size:18px;' title='Show/Hide Password'>&#128065;</span>";
      html += "<span id='current-password-real' style='display:none;font-family:monospace;'>" + web_password + "</span>";
      html += "</div>";
      html += "</div>";
      html += "</div>";

      html += "<form id='password-form' onsubmit='saveWebPassword(event)'>";
      html += "<label>New Password:</label>";
      html += "<div style='position:relative;'>";
      html += "<input type='password' id='new-password' placeholder='Enter new password' required style='width:100%;padding:8px;padding-right:40px;margin:5px 0;box-sizing:border-box;'>";
      html += "<span onclick='togglePasswordField(\"new-password\")' style='position:absolute;right:10px;top:50%;transform:translateY(-50%);cursor:pointer;font-size:18px;' title='Show/Hide'>&#128065;</span>";
      html += "</div>";
      html += "<label>Confirm Password:</label>";
      html += "<div style='position:relative;'>";
      html += "<input type='password' id='confirm-password' placeholder='Confirm new password' required style='width:100%;padding:8px;padding-right:40px;margin:5px 0;box-sizing:border-box;'>";
      html += "<span onclick='togglePasswordField(\"confirm-password\")' style='position:absolute;right:10px;top:50%;transform:translateY(-50%);cursor:pointer;font-size:18px;' title='Show/Hide'>&#128065;</span>";
      html += "</div>";
      html += "<p style='font-size:0.85em;color:#666;margin:5px 0;'>Password must be at least 4 characters</p>";
      html += "<button type='submit' class='btn btn-success' style='width:100%;margin-top:10px;'>Change Password</button>";
      html += "</form>";
      html += "</div>";
    }
    
    // NETWORK SUBMENU
    if (submenu == "network") {
      // MQTT Configuration Card
      html += "<div class='card'>";
      html += "<h3>MQTT Configuration</h3>";
      html += "<p>Configure MQTT pub/sub for real-time data streaming</p>";
      html += "<form id='mqtt-form' onsubmit='saveMqttConfig(event)'>";

      // Enable/Disable
      html += "<label style='display:flex;align-items:center;gap:10px;cursor:pointer;margin:15px 0;'>";
      html += "<input type='checkbox' id='mqtt-enabled' " + String(mqtt_enabled ? "checked" : "") + " style='width:20px;height:20px;cursor:pointer;'>";
      html += "<span><strong>Enable MQTT</strong></span>";
      html += "</label>";

      // Broker
      html += "<label>MQTT Broker:</label>";
      html += "<input type='text' id='mqtt-broker' value='" + mqtt_broker + "' placeholder='e.g., broker.hivemq.com or 192.168.1.100' style='width:100%;padding:8px;margin:5px 0;box-sizing:border-box;'>";

      // Port
      html += "<label>Port:</label>";
      html += "<input type='number' id='mqtt-port' value='" + String(mqtt_port) + "' min='1' max='65535' placeholder='1883' style='width:100%;padding:8px;margin:5px 0;box-sizing:border-box;'>";

      // Username
      html += "<label>Username (optional):</label>";
      html += "<input type='text' id='mqtt-username' value='" + mqtt_username + "' placeholder='Leave empty if no auth' style='width:100%;padding:8px;margin:5px 0;box-sizing:border-box;'>";

      // Password
      html += "<label>Password (optional):</label>";
      html += "<div style='position:relative;'>";
      html += "<input type='password' id='mqtt-password' value='" + mqtt_password + "' placeholder='Leave empty if no auth' style='width:100%;padding:8px;padding-right:40px;margin:5px 0;box-sizing:border-box;'>";
      html += "<span onclick='togglePasswordField(\"mqtt-password\")' style='position:absolute;right:10px;top:50%;transform:translateY(-50%);cursor:pointer;font-size:18px;' title='Show/Hide'>&#128065;</span>";
      html += "</div>";

      // Client ID
      html += "<label>Client ID:</label>";
      html += "<input type='text' id='mqtt-client-id' value='" + mqtt_client_id + "' placeholder='Default: your callsign' style='width:100%;padding:8px;margin:5px 0;box-sizing:border-box;'>";

      // Topic Prefix
      html += "<label>Topic Prefix:</label>";
      html += "<input type='text' id='mqtt-topic-prefix' value='" + mqtt_topic_prefix + "' placeholder='Default: " + device_hostname + "/" + dmr_callsign + "' style='width:100%;padding:8px;margin:5px 0;box-sizing:border-box;'>";

      // Publish Interval
      html += "<label>Publish Interval (ms):</label>";
      html += "<input type='number' id='mqtt-interval' value='" + String(mqtt_publish_interval) + "' min='5000' max='300000' placeholder='30000' style='width:100%;padding:8px;margin:5px 0;box-sizing:border-box;'>";
      html += "<p style='font-size:0.85em;color:#666;margin:5px 0;'>How often to publish system/modem/network status (5000-300000 ms)</p>";

      // Submit button
      html += "<button type='submit' class='btn btn-success' style='width:100%;margin-top:10px;'>Save MQTT Config</button>";
      html += "</form>";
      html += "</div>";
    }
    
    // MAINTENANCE SUBMENU
    if (submenu == "maintenance") {
      // Configuration Management Card
      html += "<div class='card'>";
      html += "<h3>Configuration Management</h3>";
      html += "<p>Manage system configuration:</p>";
      html += "<div class='action-buttons-vertical'>";
      html += "<a href='/showprefs' class='btn btn-primary'>Show Preferences</a>";
      html += "<a href='javascript:void(0)' onclick='downloadConfig()' class='btn btn-success'>Export Config</a>";
      html += "<a href='javascript:void(0)' onclick='document.getElementById(\"config-file\").click()' class='btn btn-info'>Import Config</a>";
      html += "<a href='javascript:void(0)' onclick='cleanupPrefs()' class='btn btn-warning'>Repair Preferences</a>";
      html += "<a href='/resetconfig' class='btn btn-danger'>Reset All Settings</a>";
      html += "</div>";
      html += "<input type='file' id='config-file' accept='.txt,.cfg,.conf' style='display: none;'>";
      html += "</div>";

      // OTA Update Card
      html += "<div class='card'>";
      html += "<h3>ESP32 Firmware</h3>";
      html += "<div><strong>Current Version:</strong> " + firmwareVersion + "</div>";
      html += "<div><strong>Build Date:</strong> " + String(__DATE__) + " " + String(__TIME__) + "</div>";
      html += "<br>";
      html += "<div><strong>Stable Version:</strong> <span id='latest-version'>Checking...</span></div>";
      html += "<div><strong>Beta Version:</strong> <span id='latest-beta-version'>Checking...</span></div>";
      html += "<br>";
      html += "<div id='update-status-text' style='text-align: center; font-size: 0.9em; display: flex; justify-content: center;'></div>";
      html += "<p>Over-the-Air (OTA) firmware update options:</p>";
      html += "<div style='margin-bottom: 10px;'>";
      html += "<label for='version-select' style='display: block; margin-bottom: 5px; font-weight: bold;'>Update Version:</label>";
      html += "<select id='version-select' style='width: 100%; padding: 8px; border: 1px solid var(--border-color); border-radius: 4px; background: var(--input-bg); color: var(--text-color);'>";
      // Auto-select beta if current version contains _BETA
      bool isBetaVersion = String(firmwareVersion).indexOf("_BETA") != -1;
      if (isBetaVersion) {
        html += "<option value='stable'>Stable Release</option>";
        html += "<option value='beta' selected>Beta Release</option>";
        html += "<option value='factory'>Factory Setup</option>";
      } else {
        html += "<option value='stable' selected>Stable Release</option>";
        html += "<option value='beta'>Beta Release</option>";
        html += "<option value='factory'>Factory Setup</option>";
      }
      html += "</select>";
      html += "</div>";
      html += "<div class='action-buttons-vertical'>";
      html += "<a href='javascript:void(0)' onclick='startOnlineUpdate()' class='btn btn-success'>Online Update</a>";
      html += "<a href='javascript:void(0)' onclick='document.getElementById(\"firmware-file\").click()' class='btn btn-primary'>Upload File</a>";
      html += "</div>";
      html += "<input type='file' id='firmware-file' accept='.bin' style='display: none;' />";
      html += "<div id='update-status' style='margin-top: 10px; padding: 10px; display: none;'></div>";
      html += "</div>";

      // MMDVM Modem Flasher Card
      html += "<div class='card'>";
      html += "<h3>MMDVM Modem Firmware</h3>";
      html += "<div><strong>Current Modem Version:</strong> <span id='modem-version'>" + modemFirmwareVersion + "</span></div>";
      html += "<br>";
      html += "<p>Flash firmware to your MMDVM modem (STM32):</p>";
      html += "<div style='margin-bottom: 10px;'>";
      html += "<label for='modem-firmware-select' style='display: block; margin-bottom: 5px; font-weight: bold;'>Firmware Version:</label>";
      html += "<select id='modem-firmware-select' style='width: 100%; padding: 8px; border: 1px solid var(--border-color); border-radius: 4px; background: var(--input-bg); color: var(--text-color);'>";
      html += "<option value=''>Select firmware version...</option>";
      html += "<option value='https://github.com/javastraat/esp32_mmdvm_hotspot/raw/refs/heads/main/firmware/mmdvm/mmdvm_hs_hat_fw.bin'>Single MMDVM Modem v1.6.1</option>";
      html += "<option value='https://github.com/javastraat/esp32_mmdvm_hotspot/raw/refs/heads/main/firmware/mmdvm/mmdvm_hs_dual_hat_fw.bin'>Dual MMDVM Modem v1.6.1</option>";
      html += "<option value='https://github.com/javastraat/esp32_mmdvm_hotspot/raw/refs/heads/main/firmware/mmdvm/generic_gpio_fw152.bin'>Single MMDVM Modem v1.5.2</option>";
      html += "<option value='custom'>Enter custom URL...</option>";
      html += "</select>";
      html += "</div>";
      html += "<input type='text' id='modem-custom-url' placeholder='Enter custom firmware URL...' style='display: none; width: 100%; padding: 8px; margin: 10px 0; border: 1px solid var(--border-color); border-radius: 4px; background: var(--input-bg); color: var(--text-color);' />";
      html += "<div class='action-buttons-vertical'>";
      html += "<a href='javascript:void(0)' onclick='flashModemFromURL()' class='btn btn-success'>Download & Flash</a>";
      html += "<a href='javascript:void(0)' onclick='document.getElementById(\"modem-file-input\").click()' class='btn btn-primary'>Upload File</a>";
      html += "<a href='javascript:void(0)' onclick='testMmdvm()' class='btn btn-info'>Test MMDVM</a>";
      html += "</div>";
      html += "<input type='file' id='modem-file-input' accept='.bin' style='display: none;' />";
      
      html += "<div id='modem-flash-progress' style='display:none; margin: 15px 0;'>";
      html += "<div style='background: var(--border-color, #ddd); border-radius: 4px; height: 30px; position: relative; box-shadow: inset 0 1px 3px rgba(0,0,0,0.1);'>";
      html += "<div id='modem-progress-bar' style='background: linear-gradient(90deg, #28a745, #34ce57); height: 100%; border-radius: 4px; width: 0%; transition: width 0.3s; box-shadow: 0 2px 4px rgba(40,167,69,0.3);'></div>";
      html += "<div id='modem-progress-text' style='position: absolute; width: 100%; text-align: center; line-height: 30px; color: var(--text-color, #000); font-weight: bold; text-shadow: 0 0 2px rgba(255,255,255,0.8);'>0%</div>";
      html += "</div>";
      html += "</div>";
      
      html += "<div id='modem-flash-status' style='margin-top: 10px; padding: 10px; display: none; border-radius: 4px;'></div>";
      html += "</div>";
    }

    html += "</div>"; // Close admin-grid
  }

  // Warning message (only show in submenus)
  if (submenu != "") {
    html += "<div class='info' style='background: var(--info-bg); border-left-color: #ffc107; color: var(--text-color);'>";
    html += "<strong>Warning:</strong> Some actions like reset and reboot will cause the system to restart. ";
    html += "Make sure you have saved any important configuration changes before proceeding.";
    html += "</div>";
  }

  html += "<script>";
  html += "function saveHostname(event) {";
  html += "  event.preventDefault();";
  html += "  var hostname = document.getElementById('hostname-input').value;";
  html += "  if (hostname && /^[a-zA-Z0-9-]{1,32}$/.test(hostname)) {";
  html += "    fetch('/save-hostname', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'hostname=' + encodeURIComponent(hostname)}).then(response => response.text()).then(data => {";
  html += "      if (data.includes('SUCCESS')) {";
  html += "        alert('Hostname saved! System will reboot in 3 seconds.\\n\\nNew URL: http://' + hostname + '.local');";
  html += "        setTimeout(() => { window.location.href = 'http://' + hostname + '.local'; }, 3000);";
  html += "      } else {";
  html += "        alert('Error: ' + data);";
  html += "      }";
  html += "    });";
  html += "  } else {";
  html += "    alert('Invalid hostname. Use only letters, numbers, and hyphens (1-32 characters).');";
  html += "  }";
  html += "}";
  html += "function saveVerboseLogging(event) {";
  html += "  event.preventDefault();";
  html += "  var verbose = document.getElementById('verbose-checkbox').checked ? '1' : '0';";
  html += "  fetch('/save-verbose', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'verbose=' + verbose}).then(response => response.text()).then(data => {";
  html += "    if (data.includes('SUCCESS')) {";
  html += "      alert('Verbose logging setting saved!');";
  html += "      location.reload();";
  html += "    } else {";
  html += "      alert('Error: ' + data);";
  html += "    }";
  html += "  });";
  html += "}";
  html += "function saveDebugSettings(event) {";
  html += "  event.preventDefault();";
  html += "  var verbose = document.getElementById('verbose-logging').checked ? '1' : '0';";
  html += "  var serial = document.getElementById('debug-serial').checked ? '1' : '0';";
  html += "  var mmdvm = document.getElementById('debug-mmdvm').checked ? '1' : '0';";
  html += "  var network = document.getElementById('debug-network').checked ? '1' : '0';";
  html += "  var dmr = document.getElementById('debug-dmr').checked ? '1' : '0';";
  html += "  var password = document.getElementById('debug-password').checked ? '1' : '0';";
  html += "  var body = 'verbose=' + verbose + '&serial=' + serial + '&mmdvm=' + mmdvm + '&network=' + network + '&dmr=' + dmr + '&password=' + password;";
  html += "  fetch('/save-debug', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: body}).then(response => response.text()).then(data => {";
  html += "    if (data.includes('SUCCESS')) {";
  html += "      alert('Debug settings saved!');";
  html += "      location.reload();";
  html += "    } else {";
  html += "      alert('Error: ' + data);";
  html += "    }";
  html += "  });";
  html += "}";
  html += "function saveOLEDSettings(event) {";
  html += "  event.preventDefault();";
  html += "  var oled = document.getElementById('enable-oled').checked ? '1' : '0';";
  html += "  var autoBlank = document.getElementById('auto-blank-enable').checked ? '1' : '0';";
  html += "  var blankTimeout = document.getElementById('blank-timeout').value;";
  html += "  fetch('/save-oled', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'oled=' + oled + '&autoBlank=' + autoBlank + '&blankTimeout=' + blankTimeout}).then(response => response.text()).then(data => {";
  html += "    if (data.includes('SUCCESS')) {";
  html += "      if (confirm('OLED display setting saved! Reboot required for changes to take effect.\\n\\nReboot now?')) {";
  html += "        fetch('/reboot', {method: 'POST'}).then(() => {";
  html += "          alert('Rebooting... Please wait 30 seconds and refresh the page.');";
  html += "          setTimeout(() => location.reload(), 30000);";
  html += "        });";
  html += "      } else {";
  html += "        location.reload();";
  html += "      }";
  html += "    } else {";
  html += "      alert('Error: ' + data);";
  html += "    }";
  html += "  });";
  html += "}";
  html += "function saveTimezone(event) {";
  html += "  event.preventDefault();";
  html += "  var timezone = document.getElementById('timezone-select').value;";
  html += "  var dst = document.getElementById('dst-checkbox').checked ? '3600' : '0';";
  html += "  fetch('/save-timezone', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'timezone=' + timezone + '&dst=' + dst}).then(response => response.text()).then(data => {";
  html += "    if (data.includes('SUCCESS')) {";
  html += "      alert('Timezone settings saved!\\n' + data.replace('SUCCESS: ', ''));";
  html += "      location.reload();";
  html += "    } else {";
  html += "      alert('Error: ' + data);";
  html += "    }";
  html += "  });";
  html += "}";
  html += "function toggleCurrentPassword() {";  html += "  var masked = document.getElementById('current-password-display');";
  html += "  var real = document.getElementById('current-password-real');";
  html += "  if (masked.style.display === 'none') {";
  html += "    masked.style.display = 'inline';";
  html += "    real.style.display = 'none';";
  html += "  } else {";
  html += "    masked.style.display = 'none';";
  html += "    real.style.display = 'inline';";
  html += "  }";
  html += "}";
  html += "function togglePasswordField(fieldId) {";
  html += "  var field = document.getElementById(fieldId);";
  html += "  field.type = field.type === 'password' ? 'text' : 'password';";
  html += "}";
  html += "function saveUsername(event) {";
  html += "  event.preventDefault();";
  html += "  var newUsername = document.getElementById('new-username').value.trim();";
  html += "  if (newUsername.length < 3) {";
  html += "    alert('Username must be at least 3 characters long!');";
  html += "    return;";
  html += "  }";
  html += "  if (confirm('Are you sure you want to change the web username to \"' + newUsername + '\"? You will need to log in again with the new username.')) {";
  html += "    fetch('/save-username', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'username=' + encodeURIComponent(newUsername)}).then(response => response.text()).then(data => {";
  html += "      if (data.includes('SUCCESS')) {";
  html += "        alert('Username changed successfully! Please log in again with your new username.');";
  html += "        window.location.href = '/';";
  html += "      } else {";
  html += "        alert('Error: ' + data);";
  html += "      }";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function saveWebPassword(event) {";
  html += "  event.preventDefault();";
  html += "  var newPassword = document.getElementById('new-password').value;";
  html += "  var confirmPassword = document.getElementById('confirm-password').value;";
  html += "  if (newPassword !== confirmPassword) {";
  html += "    alert('Passwords do not match!');";
  html += "    return;";
  html += "  }";
  html += "  if (newPassword.length < 4) {";
  html += "    alert('Password must be at least 4 characters long!');";
  html += "    return;";
  html += "  }";
  html += "  if (confirm('Are you sure you want to change the web password? You will need to log in again with the new password.')) {";
  html += "    fetch('/save-password', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'password=' + encodeURIComponent(newPassword)}).then(response => response.text()).then(data => {";
  html += "      if (data.includes('SUCCESS')) {";
  html += "        alert('Password changed successfully! Please log in again with your new password.');";
  html += "        window.location.href = '/';";
  html += "      } else {";
  html += "        alert('Error: ' + data);";
  html += "      }";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function saveMqttConfig(event) {";
  html += "  event.preventDefault();";
  html += "  var enabled = document.getElementById('mqtt-enabled').checked ? '1' : '0';";
  html += "  var broker = encodeURIComponent(document.getElementById('mqtt-broker').value);";
  html += "  var port = document.getElementById('mqtt-port').value;";
  html += "  var username = encodeURIComponent(document.getElementById('mqtt-username').value);";
  html += "  var password = encodeURIComponent(document.getElementById('mqtt-password').value);";
  html += "  var clientId = encodeURIComponent(document.getElementById('mqtt-client-id').value);";
  html += "  var prefix = encodeURIComponent(document.getElementById('mqtt-topic-prefix').value);";
  html += "  var interval = document.getElementById('mqtt-interval').value;";
  html += "  var body = 'enabled=' + enabled + '&broker=' + broker + '&port=' + port + '&username=' + username + '&password=' + password + '&client_id=' + clientId + '&prefix=' + prefix + '&interval=' + interval;";
  html += "  fetch('/save-mqtt-config', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: body}).then(response => response.text()).then(data => {";
  html += "    if (data.includes('SUCCESS')) {";
  html += "      alert('MQTT configuration saved successfully!');";
  html += "      location.reload();";
  html += "    } else {";
  html += "      alert('Error: ' + data);";
  html += "    }";
  html += "  });";
  html += "}";
  html += "function rebootSystem() {";
  html += "  if (confirm('Are you sure you want to reboot the system? This will temporarily interrupt service.')) {";
  html += "    fetch('/reboot', {method: 'POST'}).then(() => {";
  html += "      alert('System is rebooting... Please wait 30 seconds before reconnecting.');";
  html += "      setTimeout(() => { window.location.href = '/'; }, 30000);";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function restartServices() {";
  html += "  if (confirm('Restart DMR and network services?')) {";
  html += "    fetch('/restart-services', {method: 'POST'}).then(() => {";
  html += "      alert('Services restarted successfully!');";
  html += "      setTimeout(() => { window.location.reload(); }, 2000);";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function clearLogs() {";
  html += "  if (confirm('Clear all system logs?')) {";
  html += "    fetch('/clearlogs', {method: 'POST'}).then(() => {";
  html += "      alert('Logs cleared successfully!');";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function downloadConfig() {";
  html += "  fetch('/export-config').then(r => r.blob()).then(blob => {";
  html += "    const url = window.URL.createObjectURL(blob);";
  html += "    const a = document.createElement('a');";
  html += "    a.href = url;";
  html += "    a.download = 'mmdvm-config.txt';";
  html += "    a.click();";
  html += "    window.URL.revokeObjectURL(url);";
  html += "  });";
  html += "}";
  html += "function importConfig() {";
  html += "  var fileInput = document.getElementById('config-file');";
  html += "  var file = fileInput.files[0];";
  html += "  if (!file) return;";
  html += "  if (confirm('Import configuration from: ' + file.name + '?\\n\\nWARNING: This will overwrite ALL current settings!\\n\\nCurrent settings will be replaced with:\\n• DMR configuration\\n• WiFi networks\\n• System settings\\n• MQTT settings\\n• All preferences\\n\\nThis action CANNOT be undone!\\n\\nContinue?')) {";
  html += "    var formData = new FormData();";
  html += "    formData.append('config', file);";
  html += "    fetch('/import-config', {method: 'POST', body: formData}).then(response => response.text()).then(data => {";
  html += "      fileInput.value = '';";
  html += "      if (data.includes('SUCCESS')) {";
  html += "        alert('Configuration imported successfully! System will reboot in 3 seconds.');";
  html += "        setTimeout(() => { window.location.href = '/'; }, 3000);";
  html += "      } else {";
  html += "        alert('Import failed: ' + data);";
  html += "      }";
  html += "    }).catch(err => {";
  html += "      fileInput.value = '';";
  html += "      alert('Configuration imported successfully! System is rebooting...');";
  html += "      setTimeout(() => { window.location.href = '/'; }, 5000);";
  html += "    });";
  html += "  } else {";
  html += "    fileInput.value = '';";
  html += "  }";
  html += "}";
  html += "function testMmdvm() {";
  html += "  alert('MMDVM test started. Check the Serial Monitor for results.');";
  html += "  fetch('/test-mmdvm', {method: 'POST'});";
  html += "}";
  html += "function cleanupPrefs() {";
  html += "  if (confirm('Repair Preferences\\n\\nThis will check all 57 preference keys and add any missing ones with defaults from config.h.\\n\\nYour existing preferences will be preserved!\\n\\nContinue?')) {";
  html += "    fetch('/cleanup-prefs', {method: 'POST'})";
  html += "      .then(response => response.text())";
  html += "      .then(data => {";
  html += "        alert(data);";
  html += "        if (data.includes('Repaired')) {";
  html += "          setTimeout(() => { window.location.href = '/'; }, 3000);";
  html += "        }";
  html += "      });";
  html += "  }";
  html += "}";
  html += "function startOnlineUpdate() {";
  html += "  var selectedVersion = document.getElementById('version-select').value;";
  html += "  var versionText = selectedVersion === 'beta' ? 'BETA' : selectedVersion === 'factory' ? 'Factory Setup' : 'Stable';";
  html += "  if (confirm('Download ' + versionText + ' firmware update from GitHub? This will check for the latest version.')) {";
  html += "    document.getElementById('update-status').style.display = 'block';";
  html += "    document.getElementById('update-status').innerHTML = '<div style=\"color: #007bff;\"><strong>Downloading ' + versionText + ' firmware from GitHub...</strong><br><br><div style=\"width: 100%; background: #e9ecef; border-radius: 4px; height: 30px; margin: 10px 0; overflow: hidden;\"><div id=\"progress-bar\" style=\"width: 0%; height: 100%; background: linear-gradient(90deg, #007bff, #0056b3); transition: width 0.3s; display: flex; align-items: center; justify-content: center; color: white; font-weight: bold;\"><span id=\"progress-text\">0%</span></div></div><div id=\"progress-status\">Initializing download...</div></div>';";
  html += "    var startTime = Date.now();";
  html += "    var progressInterval = setInterval(() => {";
  html += "      var elapsed = Math.floor((Date.now() - startTime) / 1000);";
  html += "      var fakeProgress = Math.min(90, elapsed * 3);";
  html += "      document.getElementById('progress-bar').style.width = fakeProgress + '%';";
  html += "      document.getElementById('progress-text').textContent = fakeProgress + '%';";
  html += "      document.getElementById('progress-status').textContent = 'Downloading... (' + elapsed + 's)';";
  html += "    }, 1000);";
  html += "    fetch('/download-update', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'version=' + encodeURIComponent(selectedVersion)}).then(response => response.text()).then(data => {";
  html += "      clearInterval(progressInterval);";
  html += "      if (data.includes('SUCCESS')) {";
  html += "        document.getElementById('progress-bar').style.width = '100%';";
  html += "        document.getElementById('progress-text').textContent = '100%';";
  html += "        document.getElementById('progress-status').textContent = 'Download complete!';";
  html += "        setTimeout(() => {";
  html += "          if (confirm('Firmware downloaded successfully!\\n\\nSize: ' + data.split('(')[1]?.split(')')[0] + '\\n\\nFlash the new firmware now?')) {";
  html += "            confirmFlash();";
  html += "          } else {";
  html += "            document.getElementById('update-status').innerHTML = '<div style=\"color: #28a745;\">Download complete! <button onclick=\"confirmFlash()\" style=\"padding: 10px 20px; background: #dc3545; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; font-weight: bold;\">Flash Now</button></div>';";
  html += "          }";
  html += "        }, 500);";
  html += "      } else {";
  html += "        document.getElementById('update-status').innerHTML = '<div style=\"color: #dc3545;\"><strong>ERROR: Download failed</strong><br>' + data + '</div>';";
  html += "      }";
  html += "    }).catch(err => {";
  html += "      clearInterval(progressInterval);";
  html += "      document.getElementById('update-status').innerHTML = '<div style=\"color: #dc3545;\"><strong>ERROR: Network error</strong><br>' + err + '</div>';";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function uploadFirmware() {";
  html += "  var fileInput = document.getElementById('firmware-file');";
  html += "  var file = fileInput.files[0];";
  html += "  if (!file) {";
  html += "    alert('Please select a firmware file (.bin)');";
  html += "    return;";
  html += "  }";
  html += "  if (!file.name.endsWith('.bin')) {";
  html += "    alert('Please select a valid .bin firmware file');";
  html += "    return;";
  html += "  }";
  html += "  attemptUpload(file, 1, 3);";
  html += "}";
  html += "function attemptUpload(file, attempt, maxAttempts) {";
  html += "  document.getElementById('update-status').style.display = 'block';";
  html += "  var attemptText = attempt > 1 ? ' (Attempt ' + attempt + '/' + maxAttempts + ')' : '';";
  html += "  document.getElementById('update-status').innerHTML = '<div style=\"color: #007bff;\"><strong>Uploading firmware...' + attemptText + '</strong><br><br><div style=\"width: 100%; background: #e9ecef; border-radius: 4px; height: 30px; margin: 10px 0; overflow: hidden;\"><div id=\"upload-progress-bar\" style=\"width: 0%; height: 100%; background: linear-gradient(90deg, #007bff, #0056b3); transition: width 0.3s; display: flex; align-items: center; justify-content: center; color: white; font-weight: bold;\"><span id=\"upload-progress-text\">0%</span></div></div><div id=\"upload-progress-status\">Uploading ' + file.name + '...</div></div>';";
  html += "  var formData = new FormData();";
  html += "  formData.append('firmware', file);";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.upload.addEventListener('progress', function(e) {";
  html += "    if (e.lengthComputable) {";
  html += "      var percentComplete = Math.round((e.loaded / e.total) * 100);";
  html += "      document.getElementById('upload-progress-bar').style.width = percentComplete + '%';";
  html += "      document.getElementById('upload-progress-text').textContent = percentComplete + '%';";
  html += "      document.getElementById('upload-progress-status').textContent = 'Uploaded ' + Math.round(e.loaded/1024) + ' KB of ' + Math.round(e.total/1024) + ' KB';";
  html += "    }";
  html += "  });";
  html += "  xhr.onload = function() {";
  html += "    if (xhr.status === 200 && xhr.responseText.includes('SUCCESS')) {";
  html += "      document.getElementById('update-status').innerHTML = '<div style=\"color: #28a745; font-size: 18px; font-weight: bold;\">Upload Complete!</div>';";
  html += "      setTimeout(() => {";
  html += "        if (confirm('Firmware uploaded successfully!\\n\\nSize: ' + xhr.responseText.split('(')[1]?.split(')')[0] + '\\n\\nFlash the new firmware now?')) {";
  html += "          confirmFlash();";
  html += "        } else {";
  html += "          document.getElementById('update-status').innerHTML = '<div style=\"color: #28a745;\">Upload complete! <button onclick=\"confirmFlash()\" style=\"padding: 10px 20px; background: #dc3545; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; font-weight: bold;\">Flash Now</button></div>';";
  html += "        }";
  html += "      }, 500);";
  html += "    } else {";
  html += "      if (attempt < maxAttempts) {";
  html += "        document.getElementById('upload-progress-status').textContent = 'Upload failed, retrying in 2 seconds... (Attempt ' + (attempt + 1) + '/' + maxAttempts + ')';";
  html += "        setTimeout(() => attemptUpload(file, attempt + 1, maxAttempts), 2000);";
  html += "      } else {";
  html += "        document.getElementById('update-status').innerHTML = '<div style=\"color: #dc3545;\"><strong>ERROR: Upload failed after ' + maxAttempts + ' attempts</strong><br>' + xhr.responseText + '<br><br><button onclick=\"uploadFirmware()\" style=\"padding: 8px 16px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer;\">Try Again</button></div>';";
  html += "      }";
  html += "    }";
  html += "  };";
  html += "  xhr.onerror = function() {";
  html += "    if (attempt < maxAttempts) {";
  html += "      document.getElementById('upload-progress-status').textContent = 'Network error, retrying in 2 seconds... (Attempt ' + (attempt + 1) + '/' + maxAttempts + ')';";
  html += "      setTimeout(() => attemptUpload(file, attempt + 1, maxAttempts), 2000);";
  html += "    } else {";
  html += "      document.getElementById('update-status').innerHTML = '<div style=\"color: #dc3545;\"><strong>ERROR: Upload failed after ' + maxAttempts + ' attempts</strong><br>Network connection failed<br><br><button onclick=\"uploadFirmware()\" style=\"padding: 8px 16px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer;\">Try Again</button></div>';";
  html += "    }";
  html += "  };";
  html += "  xhr.open('POST', '/upload-firmware');";
  html += "  xhr.send(formData);";
  html += "}";
  html += "function confirmFlash() {";
  html += "  if (confirm('WARNING: This will flash new firmware and reboot the system.\\n\\nThe hotspot will be unavailable for 1-2 minutes during update.\\n\\nContinue with firmware flash?')) {";
  html += "    document.getElementById('update-status').innerHTML = '<div style=\"color: #ffc107;\">FLASHING FIRMWARE... DO NOT POWER OFF!</div>';";
  html += "    fetch('/flash-firmware', {method: 'POST'}).then(() => {";
  html += "      document.getElementById('update-status').innerHTML = '<div style=\"color: #28a745;\">Update completed! System rebooting...</div>';";
  html += "      setTimeout(() => { window.location.href = '/'; }, 3000);";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function checkLatestVersion() {";
  html += "  fetch('" + String(OTA_VERSION_URL) + "')";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      var latestVersion = data.trim();";
  html += "      var currentVersion = '" + firmwareVersion + "';";
  html += "      var latestSpan = document.getElementById('latest-version');";
  html += "      var statusDiv = document.getElementById('update-status-text');";
  html += "      latestSpan.innerHTML = latestVersion;";
  html += "      var currentBase = currentVersion.replace('_BETA', '');";
  html += "      var isBeta = currentVersion.includes('_BETA');";
  html += "      if (latestVersion === currentVersion) {";
  html += "        statusDiv.innerHTML = '<div style=\"background: #28a745; color: white; padding: 8px 16px; border-radius: 6px; font-weight: bold; text-align: center; display: inline-block; margin: 0 auto;\">Up to date</div>';";
  html += "      } else if (currentBase < latestVersion) {";
  html += "        statusDiv.innerHTML = '<div style=\"background: #dc3545; color: white; padding: 8px 16px; border-radius: 6px; font-weight: bold; text-align: center; display: inline-block; margin: 0 auto;\">Update available</div>';";
  html += "      } else if (isBeta) {";
  html += "        statusDiv.innerHTML = '<div style=\"background: #ffc107; color: black; padding: 8px 16px; border-radius: 6px; font-weight: bold; text-align: center; display: inline-block; margin: 0 auto;\">Beta Version</div>';";
  html += "      } else {";
  html += "        statusDiv.innerHTML = '<div style=\"background: #28a745; color: white; padding: 8px 16px; border-radius: 6px; font-weight: bold; text-align: center; display: inline-block; margin: 0 auto;\">Up to date</div>';";
  html += "      }";
  html += "    })";
  html += "    .catch(err => {";
  html += "      document.getElementById('latest-version').innerHTML = '<span style=\"color: #dc3545;\">Error checking version</span>';";
  html += "    });";
  html += "}";
  html += "function checkLatestBetaVersion() {";
  html += "  fetch('" + String(OTA_VERSION_BETA_URL) + "')";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      var latestBetaVersion = data.trim();";
  html += "      var latestBetaSpan = document.getElementById('latest-beta-version');";
  html += "      latestBetaSpan.innerHTML = latestBetaVersion;";
  html += "    })";
  html += "    .catch(err => {";
  html += "      document.getElementById('latest-beta-version').innerHTML = '<span style=\"color: #dc3545;\">Error checking version</span>';";
  html += "    });";
  html += "}";
  
  // MMDVM Modem Flasher JavaScript Functions
  html += "document.getElementById('modem-firmware-select').onchange = function() {";
  html += "  var select = this;";
  html += "  var customInput = document.getElementById('modem-custom-url');";
  html += "  if (select.value === 'custom') {";
  html += "    customInput.style.display = 'block';";
  html += "    customInput.required = true;";
  html += "  } else {";
  html += "    customInput.style.display = 'none';";
  html += "    customInput.required = false;";
  html += "  }";
  html += "};";

  html += "document.getElementById('firmware-file').onchange = function(e) {";
  html += "  var file = e.target.files[0];";
  html += "  if (!file) {";
  html += "    return;";
  html += "  }";
  html += "  if (!file.name.endsWith('.bin')) {";
  html += "    alert('Please select a valid .bin firmware file');";
  html += "    e.target.value = '';";
  html += "    return;";
  html += "  }";
  html += "  uploadFirmware();";
  html += "};";

  html += "document.getElementById('modem-file-input').onchange = function(e) {";
  html += "  var file = e.target.files[0];";
  html += "  if (!file) {";
  html += "    return;";
  html += "  }";
  html += "  if (!file.name.endsWith('.bin')) {";
  html += "    alert('Please select a valid .bin file');";
  html += "    return;";
  html += "  }";
  html += "  if (!confirm('Upload and flash modem firmware: ' + file.name + '?\\n\\nThis will update your MMDVM modem.')) {";
  html += "    e.target.value = '';";
  html += "    return;";
  html += "  }";
  html += "  var formData = new FormData();";
  html += "  formData.append('firmware', file);";
  html += "  document.getElementById('modem-flash-progress').style.display = 'block';";
  html += "  document.getElementById('modem-flash-status').style.display = 'block';";
  html += "  document.getElementById('modem-flash-status').innerHTML = '<div style=\"color: #007bff;\">Uploading firmware to ESP32...</div>';";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  var pollInterval = null;";
  html += "  var uploadComplete = false;";
  html += "  xhr.upload.onprogress = function(e) {";
  html += "    if (e.lengthComputable) {";
  html += "      var percent = Math.round((e.loaded / e.total) * 100);";
  html += "      document.getElementById('modem-progress-bar').style.width = percent + '%';";
  html += "      document.getElementById('modem-progress-text').textContent = percent + '%';";
  html += "      if (percent >= 100 && !uploadComplete) {";
  html += "        uploadComplete = true;";
  html += "        document.getElementById('modem-flash-status').innerHTML = '<div style=\"color: #007bff;\">File uploaded, flashing modem...</div>';";
  html += "        setTimeout(function() {";
  html += "          pollInterval = setInterval(function() {";
  html += "            fetch('/modem-flash-status').then(r => r.json()).then(data => {";
  html += "              console.log('Flash status:', data);";
  html += "              if (data.progress >= 100 || (data.status && data.status.toLowerCase().includes('reboot'))) {";
  html += "                clearInterval(pollInterval);";
  html += "                document.getElementById('modem-flash-progress').style.display = 'none';";
  html += "                document.getElementById('modem-flash-status').innerHTML = '<div style=\"color: #28a745; font-weight: bold;\">Flash complete! ESP32 rebooting...<br>Page will reload in 10 seconds...</div>';";
  html += "                setTimeout(() => { location.reload(); }, 10000);";
  html += "                return;";
  html += "              }";
  html += "              document.getElementById('modem-progress-bar').style.width = data.progress + '%';";
  html += "              document.getElementById('modem-progress-text').textContent = data.progress + '%';";
  html += "              if (data.status) {";
  html += "                document.getElementById('modem-flash-status').innerHTML = '<div style=\"color: #007bff;\">' + data.status + '</div>';";
  html += "              }";
  html += "              if (!data.inProgress && data.status && data.status.includes('ERROR')) {";
  html += "                clearInterval(pollInterval);";
  html += "                document.getElementById('modem-flash-status').innerHTML = '<div style=\"color: #dc3545;\">' + data.status + '</div>';";
  html += "              }";
  html += "            }).catch(err => console.error('Status poll error:', err));";
  html += "          }, 500);";
  html += "        }, 500);";
  html += "      }";
  html += "    }";
  html += "  };";
  html += "  xhr.onload = function() {};";
  html += "  xhr.onerror = function() {};";
  html += "  xhr.open('POST', '/flash-modem-upload');";
  html += "  xhr.send(formData);";
  html += "};";
  
  html += "function flashModemFromURL() {";
  html += "  var select = document.getElementById('modem-firmware-select');";
  html += "  var customInput = document.getElementById('modem-custom-url');";
  html += "  var url = select.value === 'custom' ? customInput.value : select.value;";
  html += "  if (!url) {";
  html += "    alert('Please select or enter a firmware URL');";
  html += "    return;";
  html += "  }";
  html += "  var selectedText = select.value === 'custom' ? 'Custom URL' : select.options[select.selectedIndex].text;";
  html += "  if (!confirm('Download and flash modem firmware from:\\n' + selectedText + '?\\n\\nThis will update your MMDVM modem.')) {";
  html += "    return;";
  html += "  }";
  html += "  document.getElementById('modem-flash-progress').style.display = 'block';";
  html += "  document.getElementById('modem-flash-status').style.display = 'block';";
  html += "  document.getElementById('modem-flash-status').innerHTML = '<div style=\"color: #007bff;\">Downloading firmware...</div>';";
  html += "  fetch('/flash-modem-url', {";
  html += "    method: 'POST',";
  html += "    headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "    body: 'url=' + encodeURIComponent(url)";
  html += "  });";
  html += "  var pollInterval = setInterval(function() {";
  html += "    fetch('/modem-flash-status').then(r => r.json()).then(data => {";
  html += "      console.log('Flash status:', data);";
  html += "      if (data.progress >= 100 || (data.status && data.status.toLowerCase().includes('reboot'))) {";
  html += "        clearInterval(pollInterval);";
  html += "        document.getElementById('modem-flash-progress').style.display = 'none';";
  html += "        document.getElementById('modem-flash-status').innerHTML = '<div style=\"color: #28a745; font-weight: bold;\">Flash complete! ESP32 rebooting...<br>Page will reload in 10 seconds...</div>';";
  html += "        setTimeout(() => { location.reload(); }, 10000);";
  html += "        return;";
  html += "      }";
  html += "      document.getElementById('modem-progress-bar').style.width = data.progress + '%';";
  html += "      document.getElementById('modem-progress-text').textContent = data.progress + '%';";
  html += "      if (data.status) {";
  html += "        document.getElementById('modem-flash-status').innerHTML = '<div style=\"color: #007bff;\">' + data.status + '</div>';";
  html += "      }";
  html += "      if (!data.inProgress && data.status && data.status.includes('ERROR')) {";
  html += "        clearInterval(pollInterval);";
  html += "        document.getElementById('modem-flash-status').innerHTML = '<div style=\"color: #dc3545;\">' + data.status + '</div>';";
  html += "      }";
  html += "    }).catch(err => console.error('Status poll error:', err));";
  html += "  }, 500);";
  html += "}";
  
  // Config file input event listener
  html += "document.getElementById('config-file').addEventListener('change', function(e) {";
  html += "  var file = e.target.files[0];";
  html += "  if (!file) return;";
  html += "  if (!file.name.match(/\\.(txt|cfg|conf)$/i)) {";
  html += "    alert('Please select a valid configuration file (.txt, .cfg, or .conf)');";
  html += "    e.target.value = '';";
  html += "    return;";
  html += "  }";
  html += "  importConfig();";
  html += "});";

  html += "window.onload = function() { checkLatestVersion(); checkLatestBetaVersion(); };";
  html += "</script>";

  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html; charset=UTF-8", html);
}

// Note: handleClearLogs() is defined in monitor.h


#endif // WEB_PAGES_ADMIN_H
