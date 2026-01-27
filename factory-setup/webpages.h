/*
 * webpages.h - Factory Setup Web Interface
 *
 * Minimal web interface for OTA firmware deployment
 */

#ifndef WEBPAGES_H
#define WEBPAGES_H

#include <Arduino.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

// External variables from main sketch
extern WebServer server;
extern Preferences preferences;
extern bool apMode;
extern String currentIP;
extern String savedSSID;
extern String savedPassword;

// Function to get stored preferences info
String getStoredPreferencesInfo() {
  String info = "";

  // Read preferences (namespace is already open in main sketch)
  // No need to call preferences.begin() again
  String callsign = preferences.getString("dmr_callsign", "");
  unsigned int dmrId = preferences.getUInt("dmr_id", 0);
  String dmrServer = preferences.getString("dmr_server", "");
  String hostname = preferences.getString("hostname", "");
  unsigned int rxFreq = preferences.getUInt("dmr_rx_freq", 0);
  unsigned int txFreq = preferences.getUInt("dmr_tx_freq", 0);
  unsigned char colorCode = preferences.getUChar("dmr_cc", 0);

  // Count ALL stored preferences by checking all known keys
  int totalPrefs = 0;

  // DMR Settings (15 possible)
  if (preferences.isKey("dmr_callsign")) totalPrefs++;
  if (preferences.isKey("dmr_id")) totalPrefs++;
  if (preferences.isKey("dmr_server")) totalPrefs++;
  if (preferences.isKey("dmr_password")) totalPrefs++;
  if (preferences.isKey("dmr_essid")) totalPrefs++;
  if (preferences.isKey("dmr_rx_freq")) totalPrefs++;
  if (preferences.isKey("dmr_tx_freq")) totalPrefs++;
  if (preferences.isKey("dmr_power")) totalPrefs++;
  if (preferences.isKey("dmr_cc")) totalPrefs++;
  if (preferences.isKey("dmr_lat")) totalPrefs++;
  if (preferences.isKey("dmr_lon")) totalPrefs++;
  if (preferences.isKey("dmr_height")) totalPrefs++;
  if (preferences.isKey("dmr_location")) totalPrefs++;
  if (preferences.isKey("dmr_desc")) totalPrefs++;
  if (preferences.isKey("dmr_url")) totalPrefs++;

  // WiFi Networks (15 possible - 5 networks × 3 fields)
  for (int i = 0; i < 5; i++) {
    if (preferences.isKey(("wifi" + String(i) + "_label").c_str())) totalPrefs++;
    if (preferences.isKey(("wifi" + String(i) + "_ssid").c_str())) totalPrefs++;
    if (preferences.isKey(("wifi" + String(i) + "_pass").c_str())) totalPrefs++;
  }

  // System Settings (11 possible)
  if (preferences.isKey("hostname")) totalPrefs++;
  if (preferences.isKey("verbose_log")) totalPrefs++;
  if (preferences.isKey("debug_serial")) totalPrefs++;
  if (preferences.isKey("debug_mmdvm")) totalPrefs++;
  if (preferences.isKey("debug_network")) totalPrefs++;
  if (preferences.isKey("debug_dmr")) totalPrefs++;
  if (preferences.isKey("debug_password")) totalPrefs++;
  if (preferences.isKey("enable_oled")) totalPrefs++;
  if (preferences.isKey("oled_autoblank")) totalPrefs++;
  if (preferences.isKey("oled_blank_to")) totalPrefs++;
  if (preferences.isKey("modem_type")) totalPrefs++;
  if (preferences.isKey("fw_app0")) totalPrefs++;
  if (preferences.isKey("fw_app1")) totalPrefs++;

  // Mode Settings (6 possible)
  if (preferences.isKey("mode_dmr")) totalPrefs++;
  if (preferences.isKey("mode_dstar")) totalPrefs++;
  if (preferences.isKey("mode_ysf")) totalPrefs++;
  if (preferences.isKey("mode_p25")) totalPrefs++;
  if (preferences.isKey("mode_nxdn")) totalPrefs++;
  if (preferences.isKey("mode_pocsag")) totalPrefs++;

  // Web Auth (2 possible)
  if (preferences.isKey("web_username")) totalPrefs++;
  if (preferences.isKey("web_password")) totalPrefs++;

  // MQTT Settings (8 possible)
  if (preferences.isKey("mqtt_enabled")) totalPrefs++;
  if (preferences.isKey("mqtt_broker")) totalPrefs++;
  if (preferences.isKey("mqtt_port")) totalPrefs++;
  if (preferences.isKey("mqtt_user")) totalPrefs++;
  if (preferences.isKey("mqtt_pass")) totalPrefs++;
  if (preferences.isKey("mqtt_client")) totalPrefs++;
  if (preferences.isKey("mqtt_prefix")) totalPrefs++;
  if (preferences.isKey("mqtt_interval")) totalPrefs++;

  // Build HTML
  info += "<div class='card'>";
  info += "<h3>Stored Configuration</h3>";

  if (totalPrefs == 0) {
    info += "<div class='info info-warning'>";
    info += "<strong>No Configuration Found</strong><br>";
    info += "This device has not been configured yet. After flashing the main firmware, you can configure your DMR settings via the web interface.";
    info += "</div>";
  } else {
    info += "<div style='margin-bottom: 15px; display: flex; align-items: center; justify-content: space-between; flex-wrap: wrap; gap: 10px;'>";
    info += "<div class='status-badge badge-connected'>Configured</div>";
    info += "<button onclick='clearAllSettings()' class='btn btn-danger' style='padding: 8px 16px; margin: 0;'>Clear All Settings</button>";
    info += "</div>";

    if (callsign.length() > 0) {
      info += "<div style='margin: 10px 0;'><strong>Callsign:</strong> " + callsign + "</div>";
    }
    if (dmrId > 0) {
      info += "<div style='margin: 10px 0;'><strong>DMR ID:</strong> " + String(dmrId) + "</div>";
    }
    if (dmrServer.length() > 0) {
      info += "<div style='margin: 10px 0;'><strong>DMR Server:</strong> " + dmrServer + "</div>";
    }
    if (hostname.length() > 0) {
      info += "<div style='margin: 10px 0;'><strong>Hostname:</strong> " + hostname + "</div>";
    }
    if (rxFreq > 0 && txFreq > 0) {
      info += "<div style='margin: 10px 0;'><strong>Frequency:</strong> " + String(rxFreq / 1000000.0, 4) + " MHz (RX) / " + String(txFreq / 1000000.0, 4) + " MHz (TX)</div>";
    }
    if (colorCode > 0) {
      info += "<div style='margin: 10px 0;'><strong>Color Code:</strong> " + String(colorCode) + "</div>";
    }

    info += "<div style='margin-top: 15px; padding-top: 15px; border-top: 1px solid var(--border-color);'>";
    info += "<small><strong>Total Stored Settings:</strong> " + String(totalPrefs) + "</small>";
    info += "</div>";

    info += "<div class='info info-success' style='margin-top: 15px;'>";
    info += "<strong>Configuration Preserved:</strong> Your settings will be retained when you flash the main firmware.";
    info += "</div>";
  }

  info += "</div>";

  return info;
}

String getCSS() {
  String css = "<style>";
  // CSS variables matching main version (web/common/css.h)
  css += ":root { --bg-color: #f0f0f0; --container-bg: white; --text-color: #333; --border-color: #dee2e6; --card-bg: #f8f9fa; --info-bg: #e7f3ff; --link-color: #007bff; --link-hover-color: #0056b3; --input-bg: #ffffff; }";
  css += "[data-theme='dark'] { --bg-color: #1a1a1a; --container-bg: #2d2d2d; --text-color: #ffffff; --border-color: #555; --card-bg: #3a3a3a; --info-bg: #1e3a5f; --link-color: #4da6ff; --link-hover-color: #66b3ff; --input-bg: #3a3a3a; }";
  css += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: var(--bg-color); color: var(--text-color); transition: background-color 0.3s, color 0.3s; }";
  css += ".container { max-width: 1000px; margin: 20px auto; background: var(--container-bg); padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
  css += "h1 { color: var(--text-color); border-bottom: 2px solid #007bff; padding-bottom: 10px; margin-top: 0; }";
  css += "h2 { color: var(--text-color); margin-top: 30px; }";
  css += "h3 { margin-top: 0; color: var(--text-color); border-bottom: 1px solid var(--border-color); padding-bottom: 10px; margin-bottom: 15px; }";
  // Grid layout matching main version
  css += ".admin-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 20px 0; }";
  css += ".grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 15px; margin: 20px 0; }";
  // Card styling matching main version
  css += ".card { background: var(--card-bg); padding: 15px; border-radius: 6px; border: 1px solid var(--border-color); }";
  css += ".card h3 { margin-top: 0; color: var(--text-color); }";
  // Info and status styling
  css += ".info { padding: 12px; background: var(--info-bg); border-left: 4px solid #007bff; margin: 10px 0; border-radius: 0 4px 4px 0; }";
  css += ".info-warning { background: rgba(255, 193, 7, 0.15); border-left-color: #ffc107; }";
  css += ".info-success { background: rgba(40, 167, 69, 0.15); border-left-color: #28a745; }";
  css += ".info-danger { background: rgba(220, 53, 69, 0.15); border-left-color: #dc3545; }";
  css += ".status { padding: 12px; margin: 10px 0; border-radius: 6px; font-weight: bold; }";
  css += ".status.connected { background: #d4edda; border: 1px solid #c3e6cb; color: #155724; }";
  css += ".status.disconnected { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; }";
  css += ".status.warning { background: #fff3cd; border: 1px solid #ffeaa7; color: #856404; }";
  css += ".status-badge { display: inline-block; padding: 8px 16px; border-radius: 6px; font-weight: bold; margin: 10px 0; }";
  css += ".badge-connected { background: #28a745; color: white; }";
  css += ".badge-ap { background: #ffc107; color: black; }";
  // Metric styling matching main version
  css += ".metric { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid var(--border-color); }";
  css += ".metric:last-child { border-bottom: none; }";
  css += ".metric-label { font-weight: bold; color: var(--text-color); }";
  css += ".metric-value { color: var(--text-color); }";
  // Button styling matching main version
  css += ".btn { display: inline-block; padding: 12px 24px; margin: 10px 5px; border: none; border-radius: 6px; cursor: pointer; text-decoration: none; font-size: 14px; font-weight: bold; text-align: center; transition: background-color 0.3s; }";
  css += ".btn-primary { background: #007bff; color: white; } .btn-primary:hover { background: #0056b3; }";
  css += ".btn-success { background: #28a745; color: white; } .btn-success:hover { background: #218838; }";
  css += ".btn-warning { background: #ffc107; color: black; } .btn-warning:hover { background: #e0a800; }";
  css += ".btn-danger { background: #dc3545; color: white; } .btn-danger:hover { background: #c82333; }";
  css += ".btn-info { background: #17a2b8; color: white; } .btn-info:hover { background: #138496; }";
  css += ".action-buttons-vertical { text-align: center; margin: 15px 0; }";
  css += ".action-buttons-vertical .btn { display: block; margin: 8px auto; width: 80%; }";
  // Input styling matching main version
  css += "input, select, textarea { background: var(--input-bg); color: var(--text-color); border: 1px solid var(--border-color); padding: 10px; border-radius: 4px; }";
  css += "input:focus, select:focus, textarea:focus { border-color: #007bff; outline: none; }";
  // Footer styling matching main version
  css += ".footer { text-align: center; padding: 20px 20px 5px 20px; margin-top: 30px; border-top: 1px solid var(--border-color); color: var(--text-color); font-size: 14px; }";
  css += ".footer a { color: var(--link-color); text-decoration: none; margin: 0 10px; }";
  css += ".footer a:hover { color: var(--link-hover-color); text-decoration: underline; }";
  // Theme toggle (floating button for factory-setup since no navbar)
  css += ".theme-toggle { position: fixed; top: 20px; right: 20px; background: var(--container-bg); border: 2px solid var(--border-color); border-radius: 50%; width: 50px; height: 50px; display: flex; align-items: center; justify-content: center; cursor: pointer; font-size: 24px; transition: all 0.3s; box-shadow: 0 2px 8px rgba(0,0,0,0.2); z-index: 1000; }";
  css += ".theme-toggle:hover { transform: scale(1.1); box-shadow: 0 4px 12px rgba(0,0,0,0.3); }";
  css += "</style>";
  return css;
}

String getFooter() {
  String footer = "<div class='footer'>";
  footer += "<p>" + String(COPYRIGHT_TEXT) + "</p>";
  footer += "<a href='" + String(FOOTER_LINK1_URL) + "' target='_blank'>" + String(FOOTER_LINK1_TEXT) + "</a> | ";
  footer += "<a href='" + String(FOOTER_LINK2_URL) + "' target='_blank'>" + String(FOOTER_LINK2_TEXT) + "</a> | ";
  footer += "<a href='" + String(FOOTER_LINK3_URL) + "' target='_blank'>" + String(FOOTER_LINK3_TEXT) + "</a>";
  footer += "</div>";
  return footer;
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta charset='UTF-8'>";
  html += "<title>ESP32 MMDVM - Factory Setup</title>";
  html += getCSS();
  html += "</head><body>";

  // Theme toggle button
  html += "<div class='theme-toggle' onclick='toggleTheme()' title='Toggle Dark/Light Mode'>";
  html += "<span id='theme-icon'>🌙</span>";
  html += "</div>";

  html += "<div class='container'>";

  html += "<h1>ESP32 MMDVM Hotspot</h1>";
  html += "<h2>Factory Setup & Deployment</h2>";

  html += "<div class='admin-grid'>";

  // Network Status Card
  html += "<div class='card'>";
  html += "<h3>Network Status</h3>";
  if (apMode) {
    html += "<div class='status-badge badge-ap'>Access Point Mode</div>";
    html += "<p><strong>SSID:</strong> " + String(AP_SSID) + "</p>";
    html += "<p><strong>Password:</strong> " + String(AP_PASSWORD) + "</p>";
    html += "<p><strong>IP Address:</strong> " + currentIP + "</p>";
    html += "<div class='info info-warning'>";
    html += "<strong>No Internet Connection:</strong><br>";
    html += "You are connected to the ESP32's access point, which has no internet access.<br>";
    html += "To download firmware, you must first configure WiFi settings below to connect this device to your network.";
    html += "</div>";
  } else {
    html += "<div class='status-badge badge-connected'>WiFi Connected</div>";
    // Display the actual connected SSID (from savedSSID if available, otherwise from config)
    String connectedSSID = (savedSSID.length() > 0) ? savedSSID : String(WIFI_SSID);
    html += "<p><strong>Network:</strong> " + connectedSSID + "</p>";
    html += "<p><strong>IP Address:</strong> " + currentIP + "</p>";
    html += "<div class='info' class='info info-success'>";
    html += "<strong>Internet Connected:</strong> Ready to download and flash firmware.";
    html += "</div>";
  }
  html += "</div>";

  // Stored Configuration Card
  html += getStoredPreferencesInfo();

  // WiFi Configuration Card (only show in AP mode)
  if (apMode) {
    html += "<div class='card'>";
    html += "<h3>WiFi Configuration</h3>";
    html += "<p>Connect this ESP32 to your WiFi network to enable internet access for firmware downloads.</p>";

    // Scan Networks button
    html += "<div style='margin-bottom: 15px;'>";
    html += "<button type='button' onclick='scanNetworks()' class='btn btn-primary' style='width: 100%;'>Scan for Networks</button>";
    html += "</div>";

    // Network list (hidden initially)
    html += "<div id='network-list' style='display: none; margin-bottom: 15px;'></div>";

    html += "<form id='wifi-form'>";
    html += "<div style='margin-bottom: 15px;'>";
    html += "<label style='display: block; margin-bottom: 5px; font-weight: bold;'>WiFi Network (SSID):</label>";
    html += "<input type='text' id='wifi-ssid' placeholder='Enter or select WiFi network name' style='width: 100%; padding: 10px; border: 1px solid var(--border-color); border-radius: 4px; background: var(--input-bg); color: var(--text-color);' required />";
    html += "</div>";
    html += "<div style='margin-bottom: 15px;'>";
    html += "<label style='display: block; margin-bottom: 5px; font-weight: bold;'>WiFi Password:</label>";
    html += "<input type='password' id='wifi-password' placeholder='Enter WiFi password' style='width: 100%; padding: 10px; border: 1px solid var(--border-color); border-radius: 4px; background: var(--input-bg); color: var(--text-color);' required />";
    html += "</div>";
    html += "<div class='action-buttons-vertical'>";
    html += "<button type='button' onclick='connectWiFi()' class='btn btn-primary' style='width: 100%;'>Connect to WiFi</button>";
    html += "</div>";
    html += "</form>";
    html += "<div id='wifi-status' style='margin-top: 15px; display: none;'></div>";
    html += "</div>";
  }

  // System Information Card
  html += "<div class='card'>";
  html += "<h3>System Information</h3>";
  html += "<div class='metric'><span class='metric-label'>Current Version:</span><span class='metric-value'>" + String(FACTORY_VERSION) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Chip Model:</span><span class='metric-value'>" + String(ESP.getChipModel()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Flash Size:</span><span class='metric-value'>" + String(ESP.getFlashChipSize()/1024/1024) + " MB</span></div>";
  html += "<div class='metric'><span class='metric-label'>Free Heap:</span><span class='metric-value'>" + String(ESP.getFreeHeap()/1024.0, 1) + " KB</span></div>";
  html += "</div>";

  // Partition Management Card
  html += "<div class='card'>";
  html += "<h3>Partition Management</h3>";
  html += "<p>Switch between firmware versions:</p>";

  // Get partition info
  const esp_partition_t* running = esp_ota_get_running_partition();
  const esp_partition_t* boot = esp_ota_get_boot_partition();
  const esp_partition_t* app0 = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
  const esp_partition_t* app1 = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);

  String runningLabel = running ? String(running->label) : "Unknown";
  String bootLabel = boot ? String(boot->label) : "Unknown";

  // Get firmware versions from NVS
  String app0_version = "Empty";
  String app1_version = "Empty";

  if (app0) {
    app0_version = preferences.getString("fw_app0", "Unknown");
  }
  if (app1) {
    app1_version = preferences.getString("fw_app1", "Unknown");
  }

  // For the currently running partition, use FACTORY_VERSION directly
  if (running && strcmp(running->label, "app0") == 0) {
    app0_version = String(FACTORY_VERSION);
  } else if (running && strcmp(running->label, "app1") == 0) {
    app1_version = String(FACTORY_VERSION);
  }

  html += "<div class='metric'><span class='metric-label'>Running Partition:</span><span class='metric-value'>" + runningLabel + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Boot Partition:</span><span class='metric-value'>" + bootLabel + "</span></div>";

  // Show app0 info
  if (app0) {
    String app0Label = (running && strcmp(running->label, "app0") == 0) ? "app0 (Current):" : "app0:";
    html += "<div class='metric'><span class='metric-label'>" + app0Label + "</span><span class='metric-value'>" + app0_version + "</span></div>";
  }
  // Show app1 info
  if (app1) {
    String app1Label = (running && strcmp(running->label, "app1") == 0) ? "app1 (Current):" : "app1:";
    html += "<div class='metric'><span class='metric-label'>" + app1Label + "</span><span class='metric-value'>" + app1_version + "</span></div>";
  }

  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  if (app0) {
    String app0Class = (running && strcmp(running->label, "app0") == 0) ? "btn btn-success" : "btn btn-primary";
    String app0BtnText = (running && strcmp(running->label, "app0") == 0) ? "app0 (Running)" : "Boot app0";
    html += "<a href='javascript:void(0)' onclick='switchPartition(\"app0\")' class='" + app0Class + "'>" + app0BtnText + "</a>";
  }
  if (app1) {
    String app1Class = (running && strcmp(running->label, "app1") == 0) ? "btn btn-success" : "btn btn-primary";
    String app1BtnText = (running && strcmp(running->label, "app1") == 0) ? "app1 (Running)" : "Boot app1";
    html += "<a href='javascript:void(0)' onclick='switchPartition(\"app1\")' class='" + app1Class + "'>" + app1BtnText + "</a>";
  }
  html += "</div>";
  html += "<p style='font-size:0.85em;color:var(--text-color);opacity:0.7;margin-top:10px;'>Switch partitions to rollback to a previous firmware version.</p>";
  html += "</div>";

  // MMDVM Modem Firmware Card
  html += "<div class='card'>";
  html += "<h3>MMDVM Modem Firmware</h3>";
  html += "<p>Flash firmware to your MMDVM modem (STM32-based boards like MMDVM_HS_Hat, ZUMspot, JumboSPOT)</p>";
  html += "<br>";
  html += "<div><strong>Current Modem Version:</strong> <span id='modem-version'>Detecting...</span></div>";
  html += "<br>";
  html += "<div class='info'>";
  html += "<strong>Why Flash Modem First?</strong><br>";
  html += "• Verify modem hardware is working before proceeding<br>";
  html += "• Update to latest modem firmware with bug fixes<br>";
  html += "• Test MMDVM communication independently<br>";
  html += "• One-time setup for new hardware assemblies";
  html += "</div>";
  html += "<div style='margin: 15px 0;'>";
  html += "<label for='modem-firmware-select' style='display: block; margin-bottom: 5px; font-weight: bold;'>Select Modem Firmware:</label>";
  html += "<select id='modem-firmware-select' style='width: 100%; padding: 8px; border: 1px solid var(--border-color); border-radius: 4px; background: var(--input-bg); color: var(--text-color);'>";
  html += "<option value='https://github.com/javastraat/esp32_mmdvm_hotspot/raw/refs/heads/main/firmware/mmdvm/mmdvm_hs_hat_fw.bin'>MMDVM_HS v1.6.1 (Single Hat) - Recommended</option>";
  html += "<option value='https://github.com/javastraat/esp32_mmdvm_hotspot/raw/refs/heads/main/firmware/mmdvm/mmdvm_hs_dual_hat_fw.bin'>MMDVM_HS_Dual v1.6.1 (Dual Hat)</option>";
  html += "<option value='https://github.com/javastraat/esp32_mmdvm_hotspot/raw/refs/heads/main/firmware/mmdvm/generic_gpio_fw152.bin'>MMDVM_HS v1.5.2 (Previous)</option>";
  html += "<option value='custom'>Custom URL...</option>";
  html += "</select>";
  html += "</div>";
  html += "<div id='modem-custom-url-area' style='display:none; margin-bottom: 15px;'>";
  html += "<label style='display: block; margin-bottom: 5px; font-weight: bold;'>Custom Firmware URL:</label>";
  html += "<input type='text' id='modem-custom-url' placeholder='https://github.com/.../firmware.bin' style='width: 100%; padding: 10px; border: 1px solid var(--border-color); border-radius: 4px; background: var(--input-bg); color: var(--text-color);' />";
  html += "</div>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button onclick='flashModemFromURL()' class='btn btn-success' style='width: 100%;'>Flash Modem from URL</button>";
  html += "<button onclick='document.getElementById(\"modem-firmware-file\").click()' class='btn btn-primary' style='width: 100%;'>Upload File</button>";
  html += "</div>";
  html += "<input type='file' id='modem-firmware-file' accept='.bin' style='display: none;' />";
  html += "<div id='modem-status' style='margin-top: 15px; display: none;'></div>";
  html += "</div>";

  // OTA Update Card
  html += "<div class='card'>";
  html += "<h3>Firmware Deployment</h3>";
  html += "<p>This is a factory setup tool. Use it to deploy the latest MMDVM firmware to this ESP32.</p>";
  html += "<br>";
  html += "<div><strong>Stable Version:</strong> <span id='latest-version'>Checking...</span></div>";
  html += "<div><strong>Beta Version:</strong> <span id='latest-beta-version'>Checking...</span></div>";
  html += "<br>";
  html += "<div class='info info-warning'>";
  html += "<strong>First Time Setup:</strong><br>";
  html += "1. Select your preferred firmware version (Stable recommended)<br>";
  html += "2. Click 'Download & Flash Firmware'<br>";
  html += "3. Wait for download and installation to complete<br>";
  html += "4. Device will reboot with full MMDVM firmware<br>";
  html += "5. Configure your DMR settings via the web interface";
  html += "</div>";
  html += "<div style='margin-bottom: 10px;'>";
  html += "<label for='version-select' style='display: block; margin-bottom: 5px; font-weight: bold;'>Select Firmware Version:</label>";
  html += "<select id='version-select' style='width: 100%; padding: 8px; border: 1px solid var(--border-color); border-radius: 4px; background: var(--input-bg); color: var(--text-color);'>";
  html += "<option value='stable' selected>Stable Release (Recommended)</option>";
  html += "<option value='beta'>Beta Release (Latest Features)</option>";
  html += "</select>";
  html += "</div>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='javascript:void(0)' onclick='startOnlineUpdate()' class='btn btn-success'>Download & Flash Firmware</a>";
  html += "<a href='javascript:void(0)' onclick='document.getElementById(\"firmware-file\").click()' class='btn btn-primary'>Upload File</a>";
  html += "</div>";
  html += "<input type='file' id='firmware-file' accept='.bin' style='display: none;' />";
  html += "<div id='update-status' style='margin-top: 10px; padding: 10px; display: none;'></div>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  html += "<script>";

  // Version check functions
  html += "function checkLatestVersion() {";
  html += "  fetch('" + String(OTA_VERSION_URL) + "')";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      document.getElementById('latest-version').innerHTML = data.trim();";
  html += "    })";
  html += "    .catch(err => {";
  html += "      document.getElementById('latest-version').innerHTML = '<span style=\"color: #dc3545;\">Error checking version</span>';";
  html += "    });";
  html += "}";

  html += "function checkLatestBetaVersion() {";
  html += "  fetch('" + String(OTA_VERSION_BETA_URL) + "')";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      document.getElementById('latest-beta-version').innerHTML = data.trim();";
  html += "    })";
  html += "    .catch(err => {";
  html += "      document.getElementById('latest-beta-version').innerHTML = '<span style=\"color: #dc3545;\">Error checking version</span>';";
  html += "    });";
  html += "}";

  // Online update function
  html += "function startOnlineUpdate() {";
  html += "  var selectedVersion = document.getElementById('version-select').value;";
  html += "  var versionText = selectedVersion === 'beta' ? 'BETA' : 'Stable';";
  html += "  if (confirm('Download and flash ' + versionText + ' firmware from GitHub?\\n\\nThis will replace the factory setup with the full MMDVM firmware.\\n\\nThe device will reboot after flashing.')) {";
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
  html += "          if (confirm('Firmware downloaded successfully!\\n\\nSize: ' + data.split('(')[1]?.split(')')[0] + '\\n\\nFlash the firmware now?')) {";
  html += "            confirmFlash();";
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

  // Upload function
  html += "function uploadFirmware() {";
  html += "  var fileInput = document.getElementById('firmware-file');";
  html += "  var file = fileInput.files[0];";
  html += "  if (!file) return;";
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
  html += "        if (confirm('Firmware uploaded successfully!\\n\\nSize: ' + xhr.responseText.split('(')[1]?.split(')')[0] + '\\n\\nFlash the firmware now?')) {";
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
  html += "  xhr.ontimeout = function() {";
  html += "    if (attempt < maxAttempts) {";
  html += "      document.getElementById('upload-progress-status').textContent = 'Upload timeout, retrying in 2 seconds... (Attempt ' + (attempt + 1) + '/' + maxAttempts + ')';";
  html += "      setTimeout(() => attemptUpload(file, attempt + 1, maxAttempts), 2000);";
  html += "    } else {";
  html += "      document.getElementById('update-status').innerHTML = '<div style=\"color: #dc3545;\"><strong>ERROR: Upload timed out after ' + maxAttempts + ' attempts</strong><br>File too large or connection too slow<br><br><button onclick=\"uploadFirmware()\" style=\"padding: 8px 16px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer;\">Try Again</button></div>';";
  html += "    }";
  html += "  };";
  html += "  xhr.open('POST', '/upload-firmware');";
  html += "  xhr.send(formData);";
  html += "}";

  // Flash confirmation
  html += "function confirmFlash() {";
  html += "  document.getElementById('update-status').innerHTML = '<div style=\"color: #ffc107; font-size: 18px; font-weight: bold;\">FLASHING FIRMWARE... DO NOT POWER OFF!</div>';";
  html += "  fetch('/flash-firmware', {method: 'POST'}).then(() => {";
  html += "    document.getElementById('update-status').innerHTML = '<div style=\"color: #28a745; font-size: 18px; font-weight: bold;\">Firmware flashed successfully!<br><br>Device is rebooting...<br><br>Page will reload in 15 seconds.<br>The device will restart with the full MMDVM firmware.<br>You may need to reconnect to your WiFi network.</div>';";
  html += "    setTimeout(function() { location.reload(); }, 15000);";
  html += "  });";
  html += "}";

  // Network scanning functionality
  html += "function scanNetworks() {";
  html += "  var networkList = document.getElementById('network-list');";
  html += "  networkList.style.display = 'block';";
  html += "  networkList.innerHTML = '<div style=\"color: #007bff; padding: 10px; background: #e7f3ff; border-left: 4px solid #007bff; border-radius: 4px;\"><strong>Scanning for networks...</strong><br>Please wait...</div>';";
  html += "  fetch('/scan-networks')";
  html += "    .then(response => response.json())";
  html += "    .then(networks => {";
  html += "      if (networks.length === 0) {";
  html += "        networkList.innerHTML = '<div style=\"color: #dc3545; padding: 10px; background: #f8d7da; border-left: 4px solid #dc3545; border-radius: 4px;\">No networks found</div>';";
  html += "        return;";
  html += "      }";
  html += "      var html = '<div style=\"padding: 10px; background: var(--container-bg); border: 1px solid var(--border-color); border-radius: 4px; max-height: 300px; overflow-y: auto;\">';";
  html += "      html += '<div style=\"font-weight: bold; margin-bottom: 10px; color: var(--text-color);\">Found ' + networks.length + ' networks - Click to select:</div>';";
  html += "      networks.forEach(network => {";
  html += "        var signalStrength = network.rssi;";
  html += "        var signalBars = signalStrength > -50 ? '▂▄▆█' : signalStrength > -70 ? '▂▄▆' : '▂▄';";
  html += "        var lockIcon = network.encryption !== 0 ? '[LOCK]' : '[OPEN]';";
  html += "        html += '<div onclick=\"selectNetwork(\\'' + network.ssid + '\\')\" style=\"padding: 10px; margin: 5px 0; background: var(--hover-bg); border: 1px solid var(--border-color); border-radius: 4px; cursor: pointer; transition: background 0.2s; color: var(--text-color);\" onmouseover=\"this.style.background=\\'var(--primary-color)\\'; this.style.color=\\'white\\'\" onmouseout=\"this.style.background=\\'var(--hover-bg)\\'; this.style.color=\\'var(--text-color)\\'\">';";
  html += "        html += '<strong>' + signalBars + ' ' + lockIcon + ' ' + network.ssid + '</strong><br>';";
  html += "        html += '<small>Signal: ' + network.rssi + ' dBm</small>';";
  html += "        html += '</div>';";
  html += "      });";
  html += "      html += '</div>';";
  html += "      networkList.innerHTML = html;";
  html += "    })";
  html += "    .catch(err => {";
  html += "      networkList.innerHTML = '<div style=\"color: #dc3545; padding: 10px; background: #f8d7da; border-left: 4px solid #dc3545; border-radius: 4px;\"><strong>Scan failed</strong><br>Error: ' + err.message + '</div>';";
  html += "    });";
  html += "}";
  html += "function selectNetwork(ssid) {";
  html += "  document.getElementById('wifi-ssid').value = ssid;";
  html += "  document.getElementById('wifi-password').focus();";
  html += "}";

  // WiFi connection functionality
  html += "function connectWiFi() {";
  html += "  var ssid = document.getElementById('wifi-ssid').value;";
  html += "  var password = document.getElementById('wifi-password').value;";
  html += "  if (!ssid) { alert('Please enter a WiFi network name (SSID)'); return; }";
  html += "  if (!password) { alert('Please enter a WiFi password'); return; }";
  html += "  var statusDiv = document.getElementById('wifi-status');";
  html += "  statusDiv.style.display = 'block';";
  html += "  statusDiv.innerHTML = '<div style=\"color: #007bff; padding: 10px; background: #e7f3ff; border-left: 4px solid #007bff; border-radius: 4px;\"><strong>Connecting to WiFi...</strong><br>SSID: ' + ssid + '<br>Please wait...</div>';";
  html += "  var formData = new FormData();";
  html += "  formData.append('ssid', ssid);";
  html += "  formData.append('password', password);";
  html += "  fetch('/connect-wifi', {method: 'POST', body: formData})";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      if (data.includes('SUCCESS')) {";
  html += "        var newIP = data.split('IP:')[1]?.trim() || 'unknown';";
  html += "        statusDiv.innerHTML = '<div style=\"color: #28a745; padding: 10px; background: #d4edda; border-left: 4px solid #28a745; border-radius: 4px;\"><strong>WiFi Connected!</strong><br>New IP Address: ' + newIP + '<br><br>Redirecting to new IP address in 5 seconds...<br>If not redirected, navigate to: <a href=\"http://' + newIP + '\" style=\"color: #007bff;\">http://' + newIP + '</a></div>';";
  html += "        setTimeout(() => { window.location.href = 'http://' + newIP; }, 5000);";
  html += "      } else {";
  html += "        statusDiv.innerHTML = '<div style=\"color: #dc3545; padding: 10px; background: #f8d7da; border-left: 4px solid #dc3545; border-radius: 4px;\"><strong>Connection Failed</strong><br>' + data + '<br><br><button onclick=\"connectWiFi()\" style=\"padding: 8px 16px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer;\">Try Again</button></div>';";
  html += "      }";
  html += "    })";
  html += "    .catch(err => {";
  html += "      statusDiv.innerHTML = '<div style=\"color: #dc3545; padding: 10px; background: #f8d7da; border-left: 4px solid #dc3545; border-radius: 4px;\"><strong>Connection Error</strong><br>Failed to communicate with device<br><br><button onclick=\"connectWiFi()\" style=\"padding: 8px 16px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer;\">Try Again</button></div>';";
  html += "    });";
  html += "}";

  // Theme toggle functionality
  html += "function toggleTheme() {";
  html += "  const root = document.documentElement;";
  html += "  const currentTheme = root.getAttribute('data-theme');";
  html += "  const newTheme = currentTheme === 'dark' ? 'light' : 'dark';";
  html += "  root.setAttribute('data-theme', newTheme);";
  html += "  localStorage.setItem('theme', newTheme);";
  html += "  document.getElementById('theme-icon').textContent = newTheme === 'dark' ? '☀️' : '🌙';";
  html += "}";
  html += "function initTheme() {";
  html += "  const savedTheme = localStorage.getItem('theme');";
  html += "  const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;";
  html += "  const theme = savedTheme || (prefersDark ? 'dark' : 'light');";
  html += "  document.documentElement.setAttribute('data-theme', theme);";
  html += "  document.getElementById('theme-icon').textContent = theme === 'dark' ? '☀️' : '🌙';";
  html += "}";

  // MMDVM Modem Firmware Functions
  html += "document.getElementById('modem-firmware-select').addEventListener('change', function() {";
  html += "  var customArea = document.getElementById('modem-custom-url-area');";
  html += "  customArea.style.display = this.value === 'custom' ? 'block' : 'none';";
  html += "});";

  html += "function flashModemFromURL() {";
  html += "  var select = document.getElementById('modem-firmware-select');";
  html += "  var url = select.value;";
  html += "  if (url === 'custom') {";
  html += "    url = document.getElementById('modem-custom-url').value;";
  html += "    if (!url) {";
  html += "      alert('Please enter a custom firmware URL');";
  html += "      return;";
  html += "    }";
  html += "  }";
  html += "  var firmwareName = select.options[select.selectedIndex].text;";
  html += "  if (!confirm('Flash ' + firmwareName + ' to MMDVM modem?\\n\\nThis will:\\n• Enter STM32 bootloader mode\\n• Erase modem flash memory\\n• Flash new firmware\\n• Reboot ESP32\\n\\nEnsure modem is properly connected!')) return;";
  html += "  document.getElementById('modem-status').style.display = 'block';";
  html += "  document.getElementById('modem-status').innerHTML = '<div style=\"color: #007bff;\"><strong>Starting modem flash...</strong></div>';";
  html += "  fetch('/flash-modem-url', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'url=' + encodeURIComponent(url)})";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      if (data.includes('started')) {";
  html += "        pollModemStatus();";
  html += "      } else {";
  html += "        document.getElementById('modem-status').innerHTML = '<div style=\"color: #dc3545;\"><strong>Error:</strong> ' + data + '</div>';";
  html += "      }";
  html += "    })";
  html += "    .catch(err => {";
  html += "      document.getElementById('modem-status').innerHTML = '<div style=\"color: #dc3545;\"><strong>Error:</strong> ' + err + '</div>';";
  html += "    });";
  html += "}";

  html += "function uploadModemFirmware() {";
  html += "  var fileInput = document.getElementById('modem-firmware-file');";
  html += "  var file = fileInput.files[0];";
  html += "  if (!file) return;";
  html += "  attemptModemUpload(file, 1, 3);";
  html += "}";

  html += "function attemptModemUpload(file, attempt, maxAttempts) {";
  html += "  document.getElementById('modem-status').style.display = 'block';";
  html += "  var attemptText = attempt > 1 ? ' (Attempt ' + attempt + '/' + maxAttempts + ')' : '';";
  html += "  document.getElementById('modem-status').innerHTML = '<div style=\"color: #007bff;\"><strong>Uploading modem firmware...' + attemptText + '</strong><br><br><div style=\"width: 100%; background: #e9ecef; border-radius: 4px; height: 30px; margin: 10px 0; overflow: hidden;\"><div id=\"modem-upload-progress-bar\" style=\"width: 0%; height: 100%; background: linear-gradient(90deg, #007bff, #0056b3); transition: width 0.3s; display: flex; align-items: center; justify-content: center; color: white; font-weight: bold;\"><span id=\"modem-upload-progress-text\">0%</span></div></div><div id=\"modem-upload-progress-status\">Uploading ' + file.name + '...</div></div>';";
  html += "  var formData = new FormData();";
  html += "  formData.append('firmware', file);";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  var uploadComplete = false;";
  html += "  xhr.upload.addEventListener('progress', function(e) {";
  html += "    if (e.lengthComputable) {";
  html += "      var percentComplete = Math.round((e.loaded / e.total) * 100);";
  html += "      document.getElementById('modem-upload-progress-bar').style.width = percentComplete + '%';";
  html += "      document.getElementById('modem-upload-progress-text').textContent = percentComplete + '%';";
  html += "      document.getElementById('modem-upload-progress-status').textContent = 'Uploaded ' + Math.round(e.loaded/1024) + ' KB of ' + Math.round(e.total/1024) + ' KB';";
  html += "      if (percentComplete === 100) {";
  html += "        uploadComplete = true;";
  html += "        document.getElementById('modem-upload-progress-status').textContent = 'Upload complete! Flashing modem...';";
  html += "      }";
  html += "    }";
  html += "  });";
  html += "  xhr.onload = function() {";
  html += "    if (xhr.status === 200 && xhr.responseText.includes('SUCCESS')) {";
  html += "      document.getElementById('modem-status').innerHTML = '<div style=\"color: #28a745; font-size: 18px; font-weight: bold;\">Upload Complete!</div>';";
  html += "      setTimeout(() => {";
  html += "        pollModemStatus();";
  html += "      }, 500);";
  html += "    } else {";
  html += "      if (attempt < maxAttempts) {";
  html += "        document.getElementById('modem-upload-progress-status').textContent = 'Upload failed, retrying in 2 seconds... (Attempt ' + (attempt + 1) + '/' + maxAttempts + ')';";
  html += "        setTimeout(() => attemptModemUpload(file, attempt + 1, maxAttempts), 2000);";
  html += "      } else {";
  html += "        document.getElementById('modem-status').innerHTML = '<div style=\"color: #dc3545;\"><strong>ERROR: Upload failed after ' + maxAttempts + ' attempts</strong><br>' + xhr.responseText + '<br><br><button onclick=\"uploadModemFirmware()\" style=\"padding: 8px 16px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer;\">Try Again</button></div>';";
  html += "      }";
  html += "    }";
  html += "  };";
  html += "  xhr.onerror = function() {";
  html += "    if (uploadComplete) {";
  html += "      document.getElementById('modem-status').innerHTML = '<div style=\"color: #28a745;\"><strong>Modem firmware flashing...</strong><br>ESP32 is rebooting...<br>Page will reload in 15 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 15000);";
  html += "    } else if (attempt < maxAttempts) {";
  html += "      document.getElementById('modem-upload-progress-status').textContent = 'Network error, retrying in 2 seconds... (Attempt ' + (attempt + 1) + '/' + maxAttempts + ')';";
  html += "      setTimeout(() => attemptModemUpload(file, attempt + 1, maxAttempts), 2000);";
  html += "    } else {";
  html += "      document.getElementById('modem-status').innerHTML = '<div style=\"color: #dc3545;\"><strong>ERROR: Upload failed after ' + maxAttempts + ' attempts</strong><br>Network connection failed<br><br><button onclick=\"uploadModemFirmware()\" style=\"padding: 8px 16px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer;\">Try Again</button></div>';";
  html += "    }";
  html += "  };";
  html += "  xhr.ontimeout = function() {";
  html += "    if (attempt < maxAttempts) {";
  html += "      document.getElementById('modem-upload-progress-status').textContent = 'Upload timeout, retrying in 2 seconds... (Attempt ' + (attempt + 1) + '/' + maxAttempts + ')';";
  html += "      setTimeout(() => attemptModemUpload(file, attempt + 1, maxAttempts), 2000);";
  html += "    } else {";
  html += "      document.getElementById('modem-status').innerHTML = '<div style=\"color: #dc3545;\"><strong>ERROR: Upload timed out after ' + maxAttempts + ' attempts</strong><br>File too large or connection too slow<br><br><button onclick=\"uploadModemFirmware()\" style=\"padding: 8px 16px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer;\">Try Again</button></div>';";
  html += "    }";
  html += "  };";
  html += "  xhr.timeout = 30000;";
  html += "  xhr.open('POST', '/flash-modem-upload');";
  html += "  xhr.send(formData);";
  html += "}";

  html += "function pollModemStatus() {";
  html += "  var pollInterval = setInterval(function() {";
  html += "    fetch('/flash-modem-status')";
  html += "      .then(response => response.json())";
  html += "      .then(data => {";
  html += "        var html = '<div style=\"color: #007bff;\"><strong>' + data.status + '</strong>';";
  html += "        html += '<div style=\"width: 100%; background: #e9ecef; border-radius: 4px; height: 30px; margin: 10px 0; overflow: hidden;\">';";
  html += "        html += '<div style=\"width: ' + data.progress + '%; height: 100%; background: linear-gradient(90deg, #007bff, #0056b3); transition: width 0.3s; display: flex; align-items: center; justify-content: center; color: white; font-weight: bold;\">';";
  html += "        html += '<span>' + data.progress + '%</span></div></div></div>';";
  html += "        document.getElementById('modem-status').innerHTML = html;";
  html += "        if (!data.inProgress) {";
  html += "          clearInterval(pollInterval);";
  html += "          if (data.progress === 100) {";
  html += "            document.getElementById('modem-status').innerHTML = '<div style=\"color: #28a745;\"><strong>Modem firmware flashed successfully!</strong><br>ESP32 is rebooting...<br>Page will reload in 15 seconds.</div>';";
  html += "            setTimeout(function() { location.reload(); }, 15000);";
  html += "          } else if (data.status.includes('ERROR')) {";
  html += "            document.getElementById('modem-status').innerHTML = '<div style=\"color: #dc3545;\"><strong>Flash failed:</strong> ' + data.status + '</div>';";
  html += "          }";
  html += "        }";
  html += "      })";
  html += "      .catch(err => {";
  html += "        clearInterval(pollInterval);";
  html += "        document.getElementById('modem-status').innerHTML = '<div style=\"color: #28a745;\"><strong>Connection lost - ESP32 is rebooting...</strong><br>Page will reload in 15 seconds.</div>';";
  html += "        setTimeout(function() { location.reload(); }, 15000);";
  html += "      });";
  html += "  }, 1000);";
  html += "}";

  // Get modem version function
  html += "function getModemVersion() {";
  html += "  fetch('/get-modem-version')";
  html += "    .then(response => response.text())";
  html += "    .then(version => {";
  html += "      document.getElementById('modem-version').textContent = version;";
  html += "    })";
  html += "    .catch(err => {";
  html += "      document.getElementById('modem-version').textContent = 'Detection failed';";
  html += "    });";
  html += "}";

  // File input event listeners
  html += "document.getElementById('firmware-file').addEventListener('change', function(e) {";
  html += "  var file = e.target.files[0];";
  html += "  if (!file) return;";
  html += "  if (!file.name.endsWith('.bin')) {";
  html += "    alert('Please select a valid .bin firmware file');";
  html += "    e.target.value = '';";
  html += "    return;";
  html += "  }";
  html += "  if (confirm('Upload and flash ESP32 firmware: ' + file.name + '?\\n\\nThis will replace the factory setup with full MMDVM firmware.\\nThe device will reboot after flashing.')) {";
  html += "    uploadFirmware();";
  html += "  } else {";
  html += "    e.target.value = '';";
  html += "  }";
  html += "});";

  html += "document.getElementById('modem-firmware-file').addEventListener('change', function(e) {";
  html += "  var file = e.target.files[0];";
  html += "  if (!file) return;";
  html += "  if (!file.name.endsWith('.bin')) {";
  html += "    alert('Please select a valid .bin file');";
  html += "    e.target.value = '';";
  html += "    return;";
  html += "  }";
  html += "  if (confirm('Upload and flash modem firmware: ' + file.name + '?\\n\\nThis will update your MMDVM modem.\\nThe ESP32 will reboot after flashing.')) {";
  html += "    uploadModemFirmware();";
  html += "  } else {";
  html += "    e.target.value = '';";
  html += "  }";
  html += "});";

  // Clear all settings function
  html += "function clearAllSettings() {";
  html += "  if (confirm('⚠️ WARNING: Clear ALL stored settings?\\n\\nThis will permanently delete:\\n• DMR configuration (callsign, ID, server)\\n• WiFi network credentials (all 5 networks)\\n• System settings\\n• MQTT settings\\n• Web authentication\\n• All other preferences\\n\\nTotal: 57 settings will be erased!\\n\\nThis action CANNOT be undone!\\n\\nAre you absolutely sure?')) {";
  html += "    if (confirm('FINAL CONFIRMATION\\n\\nYou are about to erase ALL device configuration!\\n\\nClick OK to proceed or Cancel to abort.')) {";
  html += "      fetch('/clear-all-settings', {method: 'POST'})";
  html += "        .then(response => response.text())";
  html += "        .then(data => {";
  html += "          if (data.includes('SUCCESS')) {";
  html += "            alert('✓ All settings cleared successfully!\\n\\nThe device will reboot in 3 seconds.');";
  html += "            setTimeout(function() { location.reload(); }, 3000);";
  html += "          } else {";
  html += "            alert('❌ Error clearing settings:\\n' + data);";
  html += "          }";
  html += "        })";
  html += "        .catch(err => {";
  html += "          alert('❌ Network error: ' + err);";
  html += "        });";
  html += "    }";
  html += "  }";
  html += "}";

  // Partition switching function
  html += "function switchPartition(partition) {";
  html += "  if (confirm('Switch boot partition to ' + partition + '?\\n\\nThe system will reboot and start from ' + partition + '.\\n\\nContinue?')) {";
  html += "    fetch('/switch-partition', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'partition=' + partition})";
  html += "      .then(r => r.text())";
  html += "      .then(msg => {";
  html += "        if (msg.includes('SUCCESS')) {";
  html += "          alert('Boot partition set to ' + partition + '. Rebooting...');";
  html += "          setTimeout(() => { window.location.href = '/'; }, 10000);";
  html += "        } else {";
  html += "          alert('Error: ' + msg);";
  html += "        }";
  html += "      })";
  html += "      .catch(err => {";
  html += "        alert('Rebooting to ' + partition + '...');";
  html += "        setTimeout(() => { window.location.href = '/'; }, 10000);";
  html += "      });";
  html += "  }";
  html += "}";

  html += "window.onload = function() { initTheme(); checkLatestVersion(); checkLatestBetaVersion(); getModemVersion(); };";
  html += "</script>";

  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html; charset=UTF-8", html);
}

#endif // WEBPAGES_H
