/*
 * System Firmware Page
 * Firmware update and version information with OTA support
 */

#ifndef WEB_SYSTEM_FIRMWARE_H
#define WEB_SYSTEM_FIRMWARE_H

#include <Arduino.h>
#include <ESP.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <Preferences.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"
#include "include/config.h"

// External references
extern String firmwareVersion;
extern String modemFirmwareVersion;

// Get current OTA settings
extern bool arduinoOtaEnabled;
extern String arduinoOtaPassword;
extern int arduinoOtaPort;

String getSystemFirmwarePageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Firmware Update - ESP32 MMDVM</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("system-firmware");

  html += "<div class='container'>";
  html += "<h1>Firmware Update</h1>";


  html += "<div class='admin-grid'>";

  // Card 1 : OTA Firmware Update
  html += "<div class='card'>";
  html += "<h3>ESP32 Firmware</h3>";
  const esp_partition_t *running = esp_ota_get_running_partition();
  size_t running_size = running ? running->size : 0;
  size_t sketch_size = ESP.getSketchSize();
  size_t ota_url_size = 0;
  String ota_url = OTA_FIRMWARE_RTOS_URL;
  String ota_url_beta = OTA_FIRMWARE_RTOS_URL_BETA;
  // Auto-switch OToA URL based on current version containing _BETA
  if (String(firmwareVersion).indexOf("_BETA") != -1) {
    ota_url = ota_url_beta;
  }
  bool isBetaVersion = String(firmwareVersion).indexOf("_BETA") != -1;
  String latestVersion = "N/A";
  /* Try to get OTA file size and latest version from server */
  #if ARDUINO_ARCH_ESP32
  {
    // Get OTA file size (Content-Length)
    HTTPClient http;
    http.begin(ota_url);
    int httpCode = http.sendRequest("HEAD");
    if (httpCode > 0) {
      ota_url_size = http.getSize();
    }
    http.end();
    // Get latest version string
    String versionUrl = isBetaVersion ? String(OTA_VERSION_BETA_URL) : String(OTA_VERSION_URL);
    http.begin(versionUrl);
    httpCode = http.GET();
    if (httpCode == 200) {
      latestVersion = http.getString();
      latestVersion.trim();
    }
    http.end();
  }
  #endif
  html += "<div class='metric'><span class='metric-label'>Chip Model:</span><span class='metric-value'>" + String(ESP.getChipModel()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Partition Size:</span><span class='metric-value'>" + String(running_size) + " bytes</span></div>";
  html += "<div class='metric'><span class='metric-label'>Version:</span><span class='metric-value'>" + firmwareVersion + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Build Date:</span><span class='metric-value'>" + String(__DATE__) + " " + String(__TIME__) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Firmware Size:</span><span class='metric-value'>" + String(sketch_size) + " bytes</span></div>";
  html += "<div class='metric'><span class='metric-label'>Remote Firmware Info:</span><span class='metric-value'></span></div>";
  
  //html += "<div style='margin-top:10px;font-size:0.85em;font-weight:bold;color:var(--text-color);'>Remote Firmware Info</div>";
  if (isBetaVersion) {
    html += "<div class='metric'><span class='metric-label'>Latest Beta:</span><span class='metric-value'>" + latestVersion + "</span></div>";
  } else {
    html += "<div class='metric'><span class='metric-label'>Latest Stable:</span><span class='metric-value'>" + latestVersion + "</span></div>";
  }
  html += "<div class='metric'><span class='metric-label'>OTA URL File Size:</span><span class='metric-value'>" + String(ota_url_size) + " bytes</span></div>";
  if (ota_url_size > 0) {
    if (sketch_size == ota_url_size) {
      html += "<div class='status-badge-wrap'><div class='status-badge badge-success'>ESP32 firmware up to date</div></div>";
      html += "<details style='margin-top:10px;'>";
    } else {
      html += "<div class='status-badge-wrap'><div class='status-badge badge-danger'>ESP32 update available</div></div>";
      html += "<details style='margin-top:10px;' open>";
    }
  } else {
    html += "<div class='status-badge-wrap'><div class='status-badge badge-warning'>OTA URL not available</div></div>";
    html += "<details style='margin-top:10px;' open>";
  }
  html += "<summary style='cursor:pointer;color:#007bff;font-size:0.9em;'>Update Options</summary>";
  html += "<div style='margin-top:8px;'>";
  html += "<p>Over-the-Air (OTA) firmware update options:</p>";
  html += "<div style='margin-bottom: 10px;'>";
  html += "<label for='version-select' style='display: block; margin-bottom: 5px; font-weight: bold;'>Update Version:</label>";
  html += "<select id='version-select' style='width: 100%; padding: 8px; border: 1px solid var(--border-color); border-radius: 4px; background: var(--input-bg); color: var(--text-color);'>";
  // Auto-select beta if current version contains _BETA
  if (isBetaVersion)
  {
    html += "<option value='stable'>Stable Release</option>";
    html += "<option value='beta' selected>Beta Release</option>";
    //html += "<option value='rtos'>RTOS Development</option>";
    html += "<option value='factory'>Factory Setup</option>";
  }
  else
  {
    html += "<option value='stable' selected>Stable Release</option>";
    html += "<option value='beta'>Beta Release</option>";
    //html += "<option value='rtos'>RTOS Development</option>";
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

  html += "<hr style='border:0;border-top:1px solid var(--border-color);margin:15px 0;'>";
  //html += "<div style='font-size:0.85em;color:#888;margin-top:5px;'>OTA URL: " + ota_url + "</div>";
  html += "</div>";
  html += "</details>";
  html += "</div>";

  // // Card 1: ESP32 Firmware OTA
  // html += "<div class='card'>";
  // html += "<h3>ESP32 Firmware</h3>";
  // html += "<div><strong>Current Version:</strong> " + firmwareVersion + "</div>";
  // html += "<div><strong>Build Date:</strong> " + String(__DATE__) + " " + String(__TIME__) + "</div>";
  // html += "<br>";
  // html += "<div><strong>Stable Version:</strong> <span id='latest-version'>Checking...</span></div>";
  // html += "<div><strong>Beta Version:</strong> <span id='latest-beta-version'>Checking...</span></div>";
  // html += "<br>";
  // html += "<div id='update-status-text' style='text-align: center; font-size: 0.9em; display: flex; justify-content: center;'></div>";
  // html += "<p>Over-the-Air (OTA) firmware update options:</p>";
  // html += "<div style='margin-bottom: 10px;'>";
  // html += "<label for='version-select' style='display: block; margin-bottom: 5px; font-weight: bold;'>Update Version:</label>";
  // html += "<select id='version-select' style='width: 100%; padding: 8px; border: 1px solid var(--border-color); border-radius: 4px; background: var(--input-bg); color: var(--text-color);'>";
  // // Auto-select beta if current version contains _BETA
  // bool isBetaVersion = String(firmwareVersion).indexOf("_BETA") != -1;
  // if (isBetaVersion)
  // {
  //   html += "<option value='stable'>Stable Release</option>";
  //   html += "<option value='beta' selected>Beta Release</option>";
  //   html += "<option value='rtos'>RTOS Development</option>";
  //   html += "<option value='factory'>Factory Setup</option>";
  // }
  // else
  // {
  //   html += "<option value='stable' selected>Stable Release</option>";
  //   html += "<option value='beta'>Beta Release</option>";
  //   html += "<option value='rtos'>RTOS Development</option>";
  //   html += "<option value='factory'>Factory Setup</option>";
  // }
  // html += "</select>";
  // html += "</div>";
  // html += "<div class='action-buttons-vertical'>";
  // html += "<a href='javascript:void(0)' onclick='startOnlineUpdate()' class='btn btn-success'>Online Update</a>";
  // html += "<a href='javascript:void(0)' onclick='document.getElementById(\"firmware-file\").click()' class='btn btn-primary'>Upload File</a>";
  // html += "</div>";
  // html += "<input type='file' id='firmware-file' accept='.bin' style='display: none;' />";
  // html += "<div id='update-status' style='margin-top: 10px; padding: 10px; display: none;'></div>";
  // html += "</div>";

  // Card 2: MMDVM Modem Firmware
  html += "<div class='card'>";
  html += "<h3>MMDVM Modem Firmware</h3>";

  // Parse modem firmware version into structured fields (same as system_status)
  if (modemFirmwareVersion.length() > 0) {
    String fw = modemFirmwareVersion;
    int dashV = fw.indexOf("-v");
    if (dashV > 0) {
      String modemHw = fw.substring(0, dashV);
      String rest = fw.substring(dashV + 1);
      String modemVer = "";
      int s1 = rest.indexOf(' ');
      if (s1 > 0) { modemVer = rest.substring(0, s1); rest = rest.substring(s1 + 1); }
      else { modemVer = rest; rest = ""; }
      String modemDate = "";
      int s2 = rest.indexOf(' ');
      if (s2 > 0) {
        modemDate = rest.substring(0, s2);
        if (modemDate.length() == 8)
          modemDate = modemDate.substring(6, 8) + "-" + modemDate.substring(4, 6) + "-" + modemDate.substring(0, 4);
        rest = rest.substring(s2 + 1);
      }
      String modemCrystal = "";
      int s3 = rest.indexOf(' ');
      if (s3 > 0) { modemCrystal = rest.substring(0, s3); rest = rest.substring(s3 + 1); }
      String modemChip = "";
      int s4 = rest.indexOf(' ');
      if (s4 > 0) { modemChip = rest.substring(0, s4); }
      String modemFwBy = "";
      int fwBy = fw.indexOf("FW by ");
      int gi = fw.indexOf("GitID ");
      if (fwBy > 0) {
        int fwByEnd = (gi > fwBy) ? gi : fw.length();
        modemFwBy = fw.substring(fwBy + 6, fwByEnd);
        modemFwBy.trim();
      }
      String modemGitId = "";
      if (gi > 0) { modemGitId = fw.substring(gi + 6); }

      html += "<div class='metric'><span class='metric-label'>Modem:</span><span class='metric-value'>" + modemHw + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Version:</span><span class='metric-value'>" + modemVer + "</span></div>";
      if (modemDate.length() > 0)
        html += "<div class='metric'><span class='metric-label'>Build Date:</span><span class='metric-value'>" + modemDate + "</span></div>";
      if (modemCrystal.length() > 0)
        html += "<div class='metric'><span class='metric-label'>Crystal:</span><span class='metric-value'>" + modemCrystal + "</span></div>";
      if (modemChip.length() > 0)
        html += "<div class='metric'><span class='metric-label'>Chip:</span><span class='metric-value'>" + modemChip + "</span></div>";
      if (modemFwBy.length() > 0)
        html += "<div class='metric'><span class='metric-label'>FW by:</span><span class='metric-value'>" + modemFwBy + "</span></div>";
      if (modemGitId.length() > 0)
        html += "<div class='metric'><span class='metric-label'>Git ID:</span><span class='metric-value'>" + modemGitId + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Latest Version:</span><span class='metric-value'>1.6.1</span></div>";
      // Compare version: extract numeric part from e.g. "v1.6.1"
      String verNum = modemVer;
      if (verNum.startsWith("v")) verNum = verNum.substring(1);
      // Parse major.minor.patch for comparison
      int dot1 = verNum.indexOf('.');
      int dot2 = verNum.indexOf('.', dot1 + 1);
      int curMajor = 0, curMinor = 0, curPatch = 0;
      if (dot1 > 0) {
        curMajor = verNum.substring(0, dot1).toInt();
        if (dot2 > 0) {
          curMinor = verNum.substring(dot1 + 1, dot2).toInt();
          curPatch = verNum.substring(dot2 + 1).toInt();
        } else {
          curMinor = verNum.substring(dot1 + 1).toInt();
        }
      }
      // Latest is 1.6.1
      if (curMajor > 1 || (curMajor == 1 && curMinor > 6) || (curMajor == 1 && curMinor == 6 && curPatch >= 1)) {
        html += "<div class='status-badge-wrap'><div class='status-badge badge-success'>Modem firmware up to date</div></div>";
        html += "<details style='margin-top:10px;'>";
      } else {
        html += "<div class='status-badge-wrap'><div class='status-badge badge-danger'>Modem update available</div></div>";
        html += "<details style='margin-top:10px;' open>";
      }
    } else {
      html += "<div class='metric'><span class='metric-label'>Firmware:</span><span class='metric-value'>" + fw + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Latest Version:</span><span class='metric-value'>1.6.1</span></div>";
      html += "<details style='margin-top:10px;' open>";
    }
  } else {
    html += "<div class='metric'><span class='metric-label'>Firmware:</span><span class='metric-value'>Not detected</span></div>";
    html += "<div class='metric'><span class='metric-label'>Latest Version:</span><span class='metric-value'>1.6.1</span></div>";
    html += "<div class='status-badge-wrap'><div class='status-badge badge-warning'>Modem not detected</div></div>";
    html += "<details style='margin-top:10px;' open>";
  }
  html += "<summary style='cursor:pointer;color:#007bff;font-size:0.9em;'>Update Options</summary>";
  html += "<div style='margin-top:8px;'>";
  html += "<div id='modem-update-status' style='text-align: center; font-size: 0.9em; display: flex; justify-content: center;'></div>";
  html += "<p>Flash firmware to your MMDVM modem (STM32):</p>";
  html += "<div style='margin-bottom: 10px;'>";
  html += "<label for='modem-firmware-select' style='display: block; margin-bottom: 5px; font-weight: bold;'>Firmware Version:</label>";
  html += "<select id='modem-firmware-select' style='width: 100%; padding: 8px; border: 1px solid var(--border-color); border-radius: 4px; background: var(--input-bg); color: var(--text-color);'>";
  html += "<option value=''>Select firmware version...</option>";
  html += "<option value='" + String(MMDVM_FIRMWARE_SINGLE_V161_URL) + "'>Single MMDVM Modem v1.6.1</option>";
  html += "<option value='" + String(MMDVM_FIRMWARE_DUAL_V161_URL) + "'>Dual MMDVM Modem v1.6.1</option>";
  html += "<option value='" + String(MMDVM_FIRMWARE_SINGLE_V152_URL) + "'>Single MMDVM Modem v1.5.2</option>";
  html += "<option value='custom'>Enter custom URL...</option>";
  html += "</select>";
  html += "</div>";
  html += "<input type='text' id='modem-custom-url' placeholder='Enter custom firmware URL...' style='display: none; width: 100%; padding: 8px; margin: 10px 0; border: 1px solid var(--border-color); border-radius: 4px; background: var(--input-bg); color: var(--text-color);' />";
  html += "<div class='action-buttons-vertical'>";
  html += "<button class='btn btn-success' onclick='flashModemFromURL()'>Download & Flash</button>";
  html += "<button class='btn btn-primary' onclick='document.getElementById(\"modem-file-input\").click()'>Upload File</button>";
  html += "<button class='btn btn-info' onclick='testMmdvm()'>Test MMDVM</button>";
  html += "</div>";
  html += "<input type='file' id='modem-file-input' accept='.bin' style='display: none;' />";

  html += "<div id='modem-flash-progress' class='progress-container' style='display:none;'>";
  html += "  <div class='progress-bar'>";
  html += "    <div id='modem-progress-bar' class='progress-fill'></div>";
  html += "    <div id='modem-progress-text' class='progress-text'>0%</div>";
  html += "  </div>";
  html += "</div>";

  html += "<div id='modem-flash-status' style='margin-top: 10px; padding: 10px; display: none; border-radius: 4px;'></div>";
  html += "</div>";
  html += "</details>";
  html += "</div>";

  // Card 3: Partition Management
  html += "<div class='card'>";
  html += "<h3>Partition Management</h3>";
  html += "<p>Switch between firmware versions:</p>";

  // Get partition info
  // 'running' already declared above
  const esp_partition_t *boot = esp_ota_get_boot_partition();
  const esp_partition_t *app0 = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
  const esp_partition_t *app1 = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);

  String runningLabel = running ? String(running->label) : "Unknown";
  String bootLabel = boot ? String(boot->label) : "Unknown";

  // Get firmware versions from NVS (stored on each boot)
  String app0_version = "Empty";
  String app1_version = "Empty";

  // Read versions from NVS
  Preferences prefs;
  prefs.begin("mmdvm", true); // Read-only
  if (app0)
  {
    app0_version = prefs.getString("fw_app0", "Unknown");
  }
  if (app1)
  {
    app1_version = prefs.getString("fw_app1", "Unknown");
  }
  prefs.end();

  // For the currently running partition, use current firmware version
  if (running && strcmp(running->label, "app0") == 0)
  {
    app0_version = firmwareVersion;
  }
  else if (running && strcmp(running->label, "app1") == 0)
  {
    app1_version = firmwareVersion;
  }

  html += "<div class='metric'><span class='metric-label'>Running Partition:</span><span class='metric-value'>" + runningLabel + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Boot Partition:</span><span class='metric-value'>" + bootLabel + "</span></div>";

  // Show app0 info
  if (app0)
  {
    String app0Label = (running && strcmp(running->label, "app0") == 0) ? "app0 (Current):" : "app0:";
    html += "<div class='metric'><span class='metric-label'>" + app0Label + "</span><span class='metric-value'>" + app0_version + "</span></div>";
  }
  // Show app1 info
  if (app1)
  {
    String app1Label = (running && strcmp(running->label, "app1") == 0) ? "app1 (Current):" : "app1:";
    html += "<div class='metric'><span class='metric-label'>" + app1Label + "</span><span class='metric-value'>" + app1_version + "</span></div>";
  }

  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  if (app0)
  {
    String app0Class = (running && strcmp(running->label, "app0") == 0) ? "btn btn-success" : "btn btn-primary";
    String app0BtnText = (running && strcmp(running->label, "app0") == 0) ? "app0 (Running)" : "Boot app0";
    html += "<a href='javascript:void(0)' onclick='switchPartition(\"app0\")' class='" + app0Class + "'>" + app0BtnText + "</a>";
  }
  if (app1)
  {
    String app1Class = (running && strcmp(running->label, "app1") == 0) ? "btn btn-success" : "btn btn-primary";
    String app1BtnText = (running && strcmp(running->label, "app1") == 0) ? "app1 (Running)" : "Boot app1";
    html += "<a href='javascript:void(0)' onclick='switchPartition(\"app1\")' class='" + app1Class + "'>" + app1BtnText + "</a>";
  }
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>Switch partitions to rollback to a previous firmware version.</p>";
  html += "</div>";

  // Card 4: ArduinoOTA Settings
  html += "<div class='card'>";
  html += "<h3>ArduinoOTA Settings</h3>";

  html += "<div class='metric'>";
  html += "<span class='metric-label'>OTA Enabled:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='ota-enabled' " + String(arduinoOtaEnabled ? "checked" : "") + ">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";

  html += "<form onsubmit='return false;'>";
  html += "<input type='text' name='username' id='ota-username' value='' autocomplete='username' style='position:absolute;left:-9999px;width:1px;height:1px;opacity:0;' tabindex='-1' aria-hidden='true'>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>OTA Password:</span>";
  html += "<input type='password' id='ota-password' value='" + arduinoOtaPassword + "' placeholder='Leave empty for no password' style='width: 120px; padding-right: 24px;' autocomplete='current-password'>";
  html += "<span id='ota-password-eye' style=\"position:absolute; right:8px; top:50%; transform:translateY(-50%); cursor:pointer;\" onclick=\"togglePasswordVisibility('ota-password', this)\"><svg width=18 height=18 viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><path d='M1 12s4-7 11-7 11 7 11 7-4 7-11 7-11-7-11-7z'/><circle cx='12' cy='12' r='3'/></svg></span>";
  html += "</div>";
  html += "</form>";
  html += "<script>";
  html += "function togglePasswordVisibility(id, icon) {";
  html += "  var input = document.getElementById(id);";
  html += "  if (input.type === 'password') {";
  html += "    input.type = 'text';";
  html += "    icon.innerHTML = \"<svg width='18' height='18' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><path d='M1 12s4-7 11-7 11 7 11 7-4 7-11 7-11-7-11-7z'/><circle cx='12' cy='12' r='3'/><line x1='1' y1='1' x2='23' y2='23'/></svg>\";";
  html += "  } else {";
  html += "    input.type = 'password';";
  html += "    icon.innerHTML = \"<svg width='18' height='18' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><path d='M1 12s4-7 11-7 11 7 11 7-4 7-11 7-11-7-11-7z'/><circle cx='12' cy='12' r='3'/></svg>\";";
  html += "  }";
  html += "}";
  if (isBetaVersion) {
    html += "window.onload = function() { checkLatestBetaVersion(); };";
  } else {
    html += "window.onload = function() { checkLatestVersion(); };";
  }
  html += "</script>";

  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>OTA Port:</span>";
  html += "<input type='text' id='ota-port' value='" + String(arduinoOtaPort) + "' style='width: 120px; padding-right: 24px;'>";
  html += "</div>";

  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>ArduinoOTA allows firmware uploads from Arduino IDE over WiFi/Ethernet.</p>";

  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' id='ota-save-btn' onclick='saveOtaSettings()'>Save</button>";
  html += "<button class='btn btn-danger' id='ota-reset-btn' onclick='resetOtaSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "<div id='ota-settings-status' style='margin-top: 10px; padding: 10px; display: none;'></div>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  // Warning message
  html += "<div class='info' style='background: var(--info-bg); border-left-color: #ffc107; color: var(--text-color);'>";
  html += "<strong>Warning:</strong> Firmware updates will cause the system to restart. ";
  html += "Make sure you have saved any important configuration changes before proceeding.";
  html += "</div>";

  // JavaScript functions
  html += "<script>";
  // Modal helpers (from system_wifi.h)
  html += "window.showModal = function(contentFn) { var overlay = document.createElement('div'); overlay.className = 'modal-overlay'; var box = document.createElement('div'); box.className = 'modal-box'; contentFn(box, function() { document.body.removeChild(overlay); }); overlay.appendChild(box); overlay.addEventListener('click', function(e) { if (e.target === overlay) document.body.removeChild(overlay); }); document.body.appendChild(overlay); return overlay; };";
  html += "window.showAlert = function(msg) { showModal(function(box, close) { box.innerHTML = '<h4>' + msg + '</h4>'; var btns = document.createElement('div'); btns.className = 'modal-buttons'; var ok = document.createElement('button'); ok.textContent = 'OK'; ok.className = 'btn btn-primary'; ok.onclick = close; btns.appendChild(ok); box.appendChild(btns); }); };";
  html += "window.showConfirm = function(msg, onYes, onNo) { showModal(function(box, close) { box.innerHTML = '<h4>' + msg + '</h4>'; var btns = document.createElement('div'); btns.className = 'modal-buttons'; var yes = document.createElement('button'); yes.textContent = 'Yes'; yes.className = 'btn btn-success'; yes.onclick = function() { close(); if(onYes)onYes(); }; var no = document.createElement('button'); no.textContent = 'Cancel'; no.className = 'btn btn-danger'; no.onclick = function() { close(); if(onNo)onNo(); }; btns.appendChild(yes); btns.appendChild(no); box.appendChild(btns); }); };";

  // ESP32 OTA Update functions
  html += "function startOnlineUpdate() {";
  html += "  var selectedVersion = document.getElementById('version-select').value;";
  html += "  var versionText = selectedVersion === 'beta' ? 'BETA' : selectedVersion === 'factory' ? 'Factory Setup' : selectedVersion === 'rtos' ? 'RTOS Development' : 'Stable';";
  html += "  showConfirm('Download ' + versionText + ' firmware update from GitHub? This will check for the latest version.', function() {";
  html += "    disableFwButtons();";
  html += "    document.getElementById('update-status').style.display = 'block';";
  html += "    document.getElementById('update-status').innerHTML = '<div class=\"status status-info\"><strong>Downloading ' + versionText + ' firmware from GitHub...</strong><br><br><div class=\"progress-bar\"><div id=\"progress-bar\" class=\"progress-fill\"></div><div id=\"progress-text\" class=\"progress-text\">0%</div></div><div id=\"progress-status\">Initializing download...</div></div>';";
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
  html += "showConfirm('Firmware downloaded successfully!\\n\\nSize: ' + data.split('(')[1]?.split(')')[0] + '\\n\\nFlash the new firmware now?', function() { confirmFlash(); }, function() { fetch('/cancel-flash',{method:'POST'}); enableFwButtons(); document.getElementById('update-status').innerHTML = '<div class=\\\"status status-success\\\">Download complete! <button onclick=\\\"confirmFlash()\\\" class=\\\"btn btn-danger\\\">Flash Now</button></div>'; });";
  //  html += "          showConfirm('Firmware downloaded successfully!\\n\\nSize: ' + data.split('(')[1]?.split(')')[0] + '\\n\\nFlash the new firmware now?', function() { confirmFlash(); }, function() { document.getElementById('update-status').innerHTML = '<div class=\\"status status-success\\">Download complete! <button onclick=\\"confirmFlash()\\" class=\\"btn btn-danger\\">Flash Now</button></div>'; });";
  html += "        }, 500);";
  html += "      } else {";
  html += "        enableFwButtons();";
  html += "        document.getElementById('update-status').innerHTML = '<div class=\"status status-danger\"><strong>ERROR: Download failed</strong><br>' + data + '</div>';";
  html += "      }";
  html += "    }).catch(err => {";
  html += "      clearInterval(progressInterval);";
  html += "      enableFwButtons();";
  html += "      document.getElementById('update-status').innerHTML = '<div class=\"status status-danger\"><strong>ERROR: Network error</strong><br>' + err + '</div>';";
  html += "    });";
  html += "  });";
  html += "}";

  html += "function disableFwButtons() {";
  html += "  document.querySelectorAll('.btn').forEach(function(b) { b.classList.add('btn-disabled'); b.style.pointerEvents = 'none'; b.style.opacity = '0.5'; });";
  html += "}";
  html += "function enableFwButtons() {";
  html += "  document.querySelectorAll('.btn').forEach(function(b) { b.classList.remove('btn-disabled'); b.style.pointerEvents = ''; b.style.opacity = ''; });";
  html += "}";

  html += "function rebootESP32() {";
  html += "  fetch('/flash-firmware', {method: 'POST'}).then(() => {";
  html += "    document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;flex-direction:column;\"><div style=\"margin-bottom:20px;font-size:32px;font-weight:bold;\">Rebooting...</div><div>Page will reload in 10 seconds.</div></div>';";
  html += "    setTimeout(() => { location.reload(); }, 10000);";
  html += "  });";
  html += "}";

  html += "function confirmFlash() {";
  html += "  showConfirm('WARNING: This will flash new firmware and reboot the system.\\n\\nThe hotspot will be unavailable for 1-2 minutes during update.\\n\\nContinue with firmware flash?', function() {";
  html += "    document.getElementById('update-status').innerHTML = '<div class=\"status status-warning\">FLASHING FIRMWARE... DO NOT POWER OFF!</div>';";
  html += "    fetch('/flash-firmware', {method: 'POST'}).then(() => {";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;flex-direction:column;\"><div style=\"margin-bottom:20px;font-size:32px;font-weight:bold;\">Rebooting...</div><div>Page will reload in 10 seconds.</div></div>';";
  html += "      setTimeout(() => { location.reload(); }, 10000);";
  html += "    });";
  html += "  }, function() { fetch('/cancel-flash',{method:'POST'}); enableFwButtons(); document.getElementById('update-status').innerHTML = '<div class=\"status status-success\">Download complete! <button onclick=\"confirmFlash()\" class=\"btn btn-danger\">Flash Now</button></div>'; });";
  html += "}";

  // Version check functions
  html += "function checkLatestVersion() {";
  html += "  fetch('" + String(OTA_VERSION_URL) + "')";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      var latestVersion = data.trim();";
  html += "      var currentVersion = '" + firmwareVersion + "';";
  html += "      var latestSpan = document.getElementById('latest-version');";
  html += "      var statusDiv = document.getElementById('update-status-text');";
  html += "      latestSpan.innerHTML = latestVersion;";
  html += "      var currentBase = currentVersion.replace('_BETA', '').replace('_RTOS', '');";
  html += "      if (latestVersion === currentVersion) {";
  html += "        statusDiv.innerHTML = '<div class=\"status-badge badge-success\">Up to date</div>';";
  html += "      } else if (currentBase < latestVersion) {";
  html += "        statusDiv.innerHTML = '<div class=\"status-badge badge-danger\">Update available</div>';";
  html += "      } else {";
  html += "        statusDiv.innerHTML = '<div class=\"status-badge badge-warning\">Development Build</div>';";
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

  // Partition switching function
  html += "function switchPartition(partition) {";
  html += "  showConfirm('Switch boot partition to ' + partition + '?\\n\\nThe system will reboot and start from ' + partition + '.\\n\\nContinue?', function() {";
  html += "    fetch('/switch-partition', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'partition=' + partition})";
  html += "      .then(r => r.text())";
  html += "      .then(msg => {";
  html += "        if (msg.includes('SUCCESS')) {";
  html += "          showAlert('Boot partition set to ' + partition + '. Rebooting...');";
  html += "          setTimeout(() => { window.location.href = '/'; }, 10000);";
  html += "        } else {";
  html += "          showAlert('Error: ' + msg);";
  html += "        }";
  html += "      })";
  html += "      .catch(err => {";
  html += "        showAlert('Rebooting to ' + partition + '...');";
  html += "        setTimeout(() => { window.location.href = '/'; }, 10000);";
  html += "      });";
  html += "  });";
  html += "}";

  html += "function uploadFirmware() {";
  html += "  var fileInput = document.getElementById('firmware-file');";
  html += "  var file = fileInput.files[0];";
  html += "  if (!file) { showAlert('Please select a firmware file (.bin)'); return; }";
  html += "  if (!file.name.endsWith('.bin')) { showAlert('Please select a valid .bin firmware file'); return; }";
  html += "  showConfirm('Upload and flash firmware: ' + file.name + ' (' + (file.size / 1024).toFixed(1) + ' KB)?', function() {";
  html += "    disableFwButtons();";
  html += "    var formData = new FormData();";
  html += "    formData.append('firmware', file);";
  html += "    document.getElementById('update-status').style.display = 'block';";
  html += "    document.getElementById('update-status').innerHTML = '<div class=\"status status-info\"><strong>Uploading firmware...</strong><br><br><div class=\"progress-bar\"><div id=\"upload-progress-bar\" class=\"progress-fill\"></div><div id=\"upload-progress-text\" class=\"progress-text\">0%</div></div></div>';";
  html += "    var xhr = new XMLHttpRequest();";
  html += "    xhr.upload.onprogress = function(e) {";
  html += "      if (e.lengthComputable) {";
  html += "        var percent = Math.round((e.loaded / e.total) * 100);";
  html += "        document.getElementById('upload-progress-bar').style.width = percent + '%';";
  html += "        document.getElementById('upload-progress-text').textContent = percent + '%';";
  html += "      }";
  html += "    };";
  html += "    xhr.onload = function() {";
  html += "      if (xhr.responseText.includes('SUCCESS')) {";
  html += "        document.getElementById('update-status').innerHTML = '<div class=\"status status-success\">' + xhr.responseText + '</div>';";
  html += "        setTimeout(() => {";
  html += "          showConfirm('Firmware uploaded successfully!\\n\\nSize: ' + (file.size / 1024).toFixed(1) + ' KB\\n\\nFlash the new firmware now?', function() { rebootESP32(); }, function() { fetch('/cancel-flash',{method:'POST'}); enableFwButtons(); document.getElementById('update-status').innerHTML = '<div class=\"status status-success\">Upload complete! <button onclick=\"rebootESP32()\" class=\"btn btn-danger\">Flash Now</button></div>'; });";
  html += "        }, 500);";
  html += "      } else {";
  html += "        enableFwButtons();";
  html += "        document.getElementById('update-status').innerHTML = '<div class=\"status status-danger\">' + xhr.responseText + '</div>';";
  html += "      }";
  html += "    };";
  html += "    xhr.onerror = function() {";
  html += "      enableFwButtons();";
  html += "      document.getElementById('update-status').innerHTML = '<div class=\"status status-danger\">Upload error</div>';";
  html += "    };";
  html += "    xhr.open('POST', '/upload-firmware?size=' + file.size);";
  html += "    xhr.send(formData);";
  html += "  });";
  html += "}";

  html += "document.getElementById('firmware-file').onchange = function(e) {";
  html += "  var file = e.target.files[0];";
  html += "  if (!file) { return; }";
  html += "  if (!file.name.endsWith('.bin')) { showAlert('Please select a valid .bin firmware file'); e.target.value = ''; return; }";
  html += "  uploadFirmware();";
  html += "};";

  html += "document.getElementById('modem-file-input').onchange = function(e) {";
  html += "  var file = e.target.files[0];";
  html += "  if (!file) { return; }";
  html += "  if (!file.name.endsWith('.bin')) { showAlert('Please select a valid .bin file'); return; }";
  html += "  showConfirm('Upload and flash modem firmware: ' + file.name + '?\\n\\nThis will update your MMDVM modem.', function() {";
  html += "    disableFwButtons();";
  html += "    var formData = new FormData();";
  html += "    formData.append('firmware', file);";
  html += "    document.getElementById('modem-flash-progress').style.display = 'block';";
  html += "    document.getElementById('modem-flash-status').style.display = 'block';";
  html += "    document.getElementById('modem-flash-status').innerHTML = '<div class=\"status status-info\">Uploading firmware to ESP32...</div>';";
  html += "    var xhr = new XMLHttpRequest();";
  html += "    xhr.upload.onprogress = function(e) {";
  html += "      if (e.lengthComputable) {";
  html += "        var percent = Math.round((e.loaded / e.total) * 100);";
  html += "        document.getElementById('modem-progress-bar').style.width = (percent * 0.1) + '%';";
  html += "        document.getElementById('modem-progress-text').textContent = 'Uploading...';";
  html += "      }";
  html += "    };";
  html += "    xhr.onload = function() {";
  html += "      if (xhr.status >= 400) {";
  html += "        enableFwButtons();";
  html += "        document.getElementById('modem-flash-status').innerHTML = '<div class=\"status status-danger\">' + xhr.responseText + '</div>';";
  html += "        return;";
  html += "      }";
  html += "      document.getElementById('modem-flash-status').innerHTML = '<div class=\"status status-info\">File uploaded, flashing modem...</div>';";
  html += "      var flashDone = false;";
  html += "      var pollInterval = setInterval(function() {";
  html += "        if (flashDone) return;";
  html += "        fetch('/flash-modem-status').then(r => r.json()).then(data => {";
  html += "          if (flashDone) return;";
  html += "          if (data.progress >= 100) {";
  html += "            flashDone = true;";
  html += "            clearInterval(pollInterval);";
  html += "            document.getElementById('modem-progress-bar').style.width = '100%';";
  html += "            document.getElementById('modem-progress-text').textContent = '100%';";
  html += "            document.getElementById('modem-flash-status').innerHTML = '<div class=\"status status-success\">' + data.status + '</div>';";
  html += "            setTimeout(() => {";
  html += "              showConfirm('Modem firmware flashed successfully!\\n\\nReboot ESP32 now to reinitialize?', function() { rebootESP32(); }, function() { fetch('/cancel-flash',{method:'POST'}); enableFwButtons(); document.getElementById('modem-flash-status').innerHTML = '<div class=\"status status-success\">Modem flash complete! <button onclick=\"rebootESP32()\" class=\"btn btn-danger\">Reboot Now</button></div>'; });";
  html += "            }, 500);";
  html += "            return;";
  html += "          }";
  html += "          document.getElementById('modem-progress-bar').style.width = data.progress + '%';";
  html += "          document.getElementById('modem-progress-text').textContent = data.progress + '%';";
  html += "          if (data.status) {";
  html += "            document.getElementById('modem-flash-status').innerHTML = '<div class=\"status status-info\">' + data.status + '</div>';";
  html += "          }";
  html += "          if (!data.inProgress && data.status && data.status.includes('ERROR')) {";
  html += "            flashDone = true;";
  html += "            clearInterval(pollInterval);";
  html += "            enableFwButtons();";
  html += "            document.getElementById('modem-flash-status').innerHTML = '<div class=\"status status-danger\">' + data.status + '</div>';";
  html += "          }";
  html += "        }).catch(err => console.error('Status poll error:', err));";
  html += "      }, 500);";
  html += "    };";
  html += "    xhr.onerror = function() {";
  html += "      enableFwButtons();";
  html += "      document.getElementById('modem-flash-status').innerHTML = '<div class=\"status status-danger\">Upload error</div>';";
  html += "    };";
  html += "    xhr.open('POST', '/flash-modem-upload?size=' + file.size);";
  html += "    xhr.send(formData);";
  html += "  });";
  html += "};";

  html += "function flashModemFromURL() {";
  html += "  var select = document.getElementById('modem-firmware-select');";
  html += "  var customInput = document.getElementById('modem-custom-url');";
  html += "  var url = select.value === 'custom' ? customInput.value : select.value;";
  html += "  if (!url) { showAlert('Please select or enter a firmware URL'); return; }";
  html += "  var selectedText = select.value === 'custom' ? 'Custom URL' : select.options[select.selectedIndex].text;";
  html += "  showConfirm('Download and flash modem firmware from:\\n' + selectedText + '?\\n\\nThis will update your MMDVM modem.', function() {";
  html += "    disableFwButtons();";
  html += "    document.getElementById('modem-flash-progress').style.display = 'block';";
  html += "    document.getElementById('modem-flash-status').style.display = 'block';";
  html += "    document.getElementById('modem-flash-status').innerHTML = '<div style=\"color: #007bff;\">Downloading firmware...</div>';";
  html += "    fetch('/flash-modem-url', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'url=' + encodeURIComponent(url)";
  html += "    });";
  html += "    var pollInterval = setInterval(function() {";
  html += "      fetch('/flash-modem-status').then(r => r.json()).then(data => {";
  html += "        console.log('Flash status:', data);";
  html += "        if (data.progress >= 100 || (data.status && data.status.toLowerCase().includes('reboot'))) {";
  html += "          clearInterval(pollInterval);";
  html += "          document.getElementById('modem-flash-progress').style.display = 'none';";
  html += "          document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;flex-direction:column;\"><div style=\"margin-bottom:20px;font-size:32px;font-weight:bold;\">Rebooting...</div><div>Page will reload in 10 seconds.</div></div>';";
  html += "          setTimeout(() => { location.reload(); }, 10000);";
  html += "          return;";
  html += "        }";
  html += "        document.getElementById('modem-progress-bar').style.width = data.progress + '%';";
  html += "        document.getElementById('modem-progress-text').textContent = data.progress + '%';";
  html += "        if (data.status) {";
  html += "          document.getElementById('modem-flash-status').innerHTML = '<div style=\"color: #007bff;\">' + data.status + '</div>';";
  html += "        }";
  html += "        if (!data.inProgress && data.status && data.status.includes('ERROR')) {";
  html += "          clearInterval(pollInterval);";
  html += "          enableFwButtons();";
  html += "          document.getElementById('modem-flash-status').innerHTML = '<div style=\"color: #dc3545;\">' + data.status + '</div>';";
  html += "        }";
  html += "      }).catch(err => console.error('Status poll error:', err));";
  html += "    }, 500);";
  html += "  });";
  html += "}";

  html += "function testMmdvm() {";
  html += "  showAlert('MMDVM test started. Check the Serial Monitor for results.');";
  html += "  fetch('/test-mmdvm', {method: 'POST'});";
  html += "}";

  html += "function saveOtaSettings() {";
  html += "  var enabled = document.getElementById('ota-enabled').checked ? '1' : '0';";
  html += "  var password = document.getElementById('ota-password').value;";
  html += "  var port = document.getElementById('ota-port').value;";
  html += "  if (port < 1 || port > 65535) { showAlert('Port must be 1-65535'); return; }";
  html += "  showConfirm('Save ArduinoOTA settings and reboot?', function() {";
  html += "    var statusDiv = document.getElementById('ota-settings-status');";
  html += "    statusDiv.style.display = 'block';";
  html += "    statusDiv.innerHTML = '<div style=\"color: #007bff;\">Saving...</div>';";
  html += "    fetch('/api/save-ota-settings', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'enabled=' + enabled + '&password=' + encodeURIComponent(password) + '&port=' + port";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      if (msg.includes('saved') || msg.includes('SUCCESS')) {";
  html += "        statusDiv.innerHTML = '<div style=\"color: #28a745;\">' + msg + '<br>Device will reboot in 10 seconds.</div>';";
  html += "        document.getElementById('ota-save-btn').disabled = true;";
  html += "        document.getElementById('ota-reset-btn').disabled = true;";
  html += "        setTimeout(function() {";
  html += "          fetch('/api/reboot', {method: 'POST'});";
  html += "          document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;flex-direction:column;\"><div style=\"margin-bottom:20px;font-size:32px;font-weight:bold;\">Rebooting...</div><div>Page will reload in 10 seconds.</div></div>';";
  html += "          setTimeout(function() { location.reload(); document.getElementById('ota-save-btn').disabled = false; document.getElementById('ota-reset-btn').disabled = false; }, 10000);";
  html += "        }, 1000);";
  html += "      } else {";
  html += "        statusDiv.innerHTML = '<div style=\"color: #dc3545;\">' + msg + '</div>';";
  html += "        document.getElementById('ota-save-btn').disabled = false;";
  html += "        document.getElementById('ota-reset-btn').disabled = false;";
  html += "      }";
  html += "    }).catch(err => {";
  html += "      statusDiv.innerHTML = '<div style=\"color: #dc3545;\">Error: ' + err + '</div>';";
  html += "      document.getElementById('ota-save-btn').disabled = false;";
  html += "      document.getElementById('ota-reset-btn').disabled = false;";
  html += "    });";
  html += "  });";
  html += "}";

  html += "function resetOtaSettings() {";
  html += "  showConfirm('Reset ArduinoOTA settings to default and reboot?', function() {";
  html += "    fetch('/api/reset-ota-settings', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '\\n\\nThe device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;flex-direction:column;\"><div style=\"margin-bottom:20px;font-size:32px;font-weight:bold;\">Rebooting...</div><div>Page will reload in 10 seconds.</div></div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  html += "</script>";

  html += getFooter();
  html += "</div>"; // Close container
  html += "</body></html>";

  return html;
}

#endif // WEB_SYSTEM_FIRMWARE_H
