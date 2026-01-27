/*
 * system_firmware.h - Firmware Update Page for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_PAGES_SYSTEM_FIRMWARE_H
#define WEB_PAGES_SYSTEM_FIRMWARE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include "../common/css.h"
#include "../common/navigation.h"
#include "../common/utils.h"
#include "../common/server_utils.h"

// External variables
extern WebServer server;
extern String firmwareVersion;
extern String modemFirmwareVersion;

void handleSystemFirmware() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Firmware Update - ESP32 MMDVM</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".admin-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 20px 0; }";
  html += ".metric { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #eee; }";
  html += ".metric:last-child { border-bottom: none; }";
  html += ".metric-label { font-weight: bold; color: #555; }";
  html += ".metric-value { color: #333; }";
  html += ".btn { display: inline-block; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; text-decoration: none; text-align: center; font-weight: bold; box-sizing: border-box; }";
  html += ".btn-success { background-color: #28a745; color: white; }";
  html += ".btn-success:hover { background-color: #218838; }";
  html += ".btn-primary { background-color: #007bff; color: white; }";
  html += ".btn-primary:hover { background-color: #0069d9; }";
  html += ".btn-info { background-color: #17a2b8; color: white; }";
  html += ".btn-info:hover { background-color: #138496; }";
  html += ".action-buttons-vertical { display: flex; flex-direction: column; gap: 10px; margin-top: 15px; }";
  html += ".action-buttons-vertical .btn { width: 100%; margin: 0; }";
  html += "</style></head><body>";
  html += getNavigation("systemfirmware");
  html += "<div class='container'>";
  html += "<h1>Firmware Update</h1>";

  html += "<div class='admin-grid'>";

  // ESP32 OTA Update Card
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

  // Get firmware versions from NVS (stored on each boot)
  // This is more reliable than esp_app_desc which contains build system info
  String app0_version = "Empty";
  String app1_version = "Empty";

  // Read versions from NVS
  Preferences prefs;
  prefs.begin("mmdvm", true);  // Read-only
  if (app0) {
    app0_version = prefs.getString("fw_app0", "Unknown");
  }
  if (app1) {
    app1_version = prefs.getString("fw_app1", "Unknown");
  }
  prefs.end();

  // For the currently running partition, use FIRMWARE_VERSION directly (most accurate)
  if (running && strcmp(running->label, "app0") == 0) {
    app0_version = String(FIRMWARE_VERSION);
  } else if (running && strcmp(running->label, "app1") == 0) {
    app1_version = String(FIRMWARE_VERSION);
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
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>Switch partitions to rollback to a previous firmware version.</p>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  // Warning message
  html += "<div class='info' style='background: var(--info-bg); border-left-color: #ffc107; color: var(--text-color);'>";
  html += "<strong>Warning:</strong> Firmware updates will cause the system to restart. ";
  html += "Make sure you have saved any important configuration changes before proceeding.";
  html += "</div>";

  // JavaScript functions
  html += "<script>";

  // Test MMDVM function
  html += "function testMmdvm() {";
  html += "  alert('MMDVM test started. Check the Serial Monitor for results.');";
  html += "  fetch('/test-mmdvm', {method: 'POST'});";
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

  // ESP32 OTA Update functions
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

  html += "window.onload = function() { checkLatestVersion(); checkLatestBetaVersion(); };";
  html += "</script>";

  html += getFooter();
  html += "</div>"; // Close container
  html += "</body></html>";

  server.send(200, "text/html; charset=UTF-8", html);
}

#endif // WEB_PAGES_SYSTEM_FIRMWARE_H
