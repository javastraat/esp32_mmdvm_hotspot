/*
 * webpages.h - Factory Setup Web Interface
 *
 * Minimal web interface for OTA firmware deployment
 */

#ifndef WEBPAGES_H
#define WEBPAGES_H

// External variables from main sketch
extern WebServer server;
extern Preferences preferences;
extern bool apMode;
extern String currentIP;
extern String savedSSID;
extern String savedPassword;

String getCSS() {
  String css = "<style>";
  css += ":root { --bg-color: #f5f5f5; --container-bg: #ffffff; --text-color: #333333; --border-color: #dddddd; --primary-color: #007bff; --primary-hover: #0056b3; --hover-bg: #f8f9fa; --input-bg: #ffffff; --info-bg: #e7f3ff; }";
  css += "[data-theme='dark'] { --bg-color: #1a1a1a; --container-bg: #2d2d2d; --text-color: #e0e0e0; --border-color: #444444; --primary-color: #4a9eff; --primary-hover: #6bb0ff; --hover-bg: #3a3a3a; --input-bg: #3a3a3a; --info-bg: #2a4a5a; }";
  css += "@media (prefers-color-scheme: dark) {";
  css += "  :root:not([data-theme='light']) { --bg-color: #1a1a1a; --container-bg: #2d2d2d; --text-color: #e0e0e0; --border-color: #444444; --primary-color: #4a9eff; --primary-hover: #6bb0ff; --hover-bg: #3a3a3a; --input-bg: #3a3a3a; --info-bg: #2a4a5a; }";
  css += "}";
  css += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  css += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: var(--bg-color); color: var(--text-color); line-height: 1.6; padding: 20px; }";
  css += ".container { max-width: 800px; margin: 0 auto; background: var(--container-bg); padding: 30px; border-radius: 12px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }";
  css += "h1 { color: var(--primary-color); margin-bottom: 10px; font-size: 2em; }";
  css += "h2 { color: var(--primary-color); margin-top: 20px; margin-bottom: 15px; font-size: 1.5em; }";
  css += "h3 { color: var(--text-color); margin-bottom: 15px; padding-bottom: 10px; border-bottom: 2px solid var(--border-color); }";
  css += ".card { background: var(--container-bg); border: 1px solid var(--border-color); border-radius: 8px; padding: 20px; margin: 20px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.05); }";
  css += ".info { padding: 15px; margin: 20px 0; background: var(--info-bg); border-left: 4px solid var(--primary-color); border-radius: 4px; }";
  css += ".status-badge { display: inline-block; padding: 8px 16px; border-radius: 6px; font-weight: bold; margin: 10px 0; }";
  css += ".badge-connected { background: #28a745; color: white; }";
  css += ".badge-ap { background: #ffc107; color: black; }";
  css += ".btn { display: inline-block; padding: 12px 24px; margin: 10px 5px; border: none; border-radius: 6px; cursor: pointer; text-decoration: none; font-size: 14px; font-weight: bold; text-align: center; transition: background-color 0.3s; }";
  css += ".btn-success { background: #28a745; color: white; } .btn-success:hover { background: #218838; }";
  css += ".btn-primary { background: #007bff; color: white; } .btn-primary:hover { background: #0056b3; }";
  css += ".btn-warning { background: #ffc107; color: black; } .btn-warning:hover { background: #e0a800; }";
  css += ".action-buttons-vertical { text-align: center; margin: 15px 0; }";
  css += ".action-buttons-vertical .btn { display: block; margin: 8px auto; width: 80%; }";
  css += "footer { margin-top: 40px; padding-top: 20px; border-top: 1px solid var(--border-color); text-align: center; font-size: 0.9em; color: var(--text-color); }";
  css += "footer a { color: var(--primary-color); text-decoration: none; margin: 0 10px; } footer a:hover { text-decoration: underline; }";
  css += ".theme-toggle { position: fixed; top: 20px; right: 20px; background: var(--container-bg); border: 2px solid var(--border-color); border-radius: 50%; width: 50px; height: 50px; display: flex; align-items: center; justify-content: center; cursor: pointer; font-size: 24px; transition: all 0.3s; box-shadow: 0 2px 8px rgba(0,0,0,0.2); z-index: 1000; }";
  css += ".theme-toggle:hover { transform: scale(1.1); box-shadow: 0 4px 12px rgba(0,0,0,0.3); }";
  css += "</style>";
  return css;
}

String getFooter() {
  String footer = "<footer>";
  footer += "<p>" + String(COPYRIGHT_TEXT) + "</p>";
  footer += "<div>";
  footer += "<a href='" + String(FOOTER_LINK1_URL) + "' target='_blank'>" + String(FOOTER_LINK1_TEXT) + "</a> | ";
  footer += "<a href='" + String(FOOTER_LINK2_URL) + "' target='_blank'>" + String(FOOTER_LINK2_TEXT) + "</a> | ";
  footer += "<a href='" + String(FOOTER_LINK3_URL) + "' target='_blank'>" + String(FOOTER_LINK3_TEXT) + "</a>";
  footer += "</div>";
  footer += "</footer>";
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

  // Network Status Card
  html += "<div class='card'>";
  html += "<h3>Network Status</h3>";
  if (apMode) {
    html += "<div class='status-badge badge-ap'>Access Point Mode</div>";
    html += "<p><strong>SSID:</strong> " + String(AP_SSID) + "</p>";
    html += "<p><strong>Password:</strong> " + String(AP_PASSWORD) + "</p>";
    html += "<p><strong>IP Address:</strong> " + currentIP + "</p>";
    html += "<div class='info' style='background: #fff3cd; border-left-color: #ffc107; color: #000;'>";
    html += "<strong>⚠️ No Internet Connection:</strong><br>";
    html += "You are connected to the ESP32's access point, which has no internet access.<br>";
    html += "To download firmware, you must first configure WiFi settings below to connect this device to your network.";
    html += "</div>";
  } else {
    html += "<div class='status-badge badge-connected'>WiFi Connected</div>";
    // Display the actual connected SSID (from savedSSID if available, otherwise from config)
    String connectedSSID = (savedSSID.length() > 0) ? savedSSID : String(WIFI_SSID);
    html += "<p><strong>Network:</strong> " + connectedSSID + "</p>";
    html += "<p><strong>IP Address:</strong> " + currentIP + "</p>";
    html += "<div class='info' style='background: #d4edda; border-left-color: #28a745; color: #000;'>";
    html += "<strong>✓ Internet Connected:</strong> Ready to download and flash firmware.";
    html += "</div>";
  }
  html += "</div>";

  // WiFi Configuration Card (only show in AP mode)
  if (apMode) {
    html += "<div class='card'>";
    html += "<h3>WiFi Configuration</h3>";
    html += "<p>Connect this ESP32 to your WiFi network to enable internet access for firmware downloads.</p>";

    // Scan Networks button
    html += "<div style='margin-bottom: 15px;'>";
    html += "<button type='button' onclick='scanNetworks()' class='btn btn-primary' style='width: 100%;'>📡 Scan for Networks</button>";
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
  html += "<div><strong>Current Version:</strong> " + String(FACTORY_VERSION) + "</div>";
  html += "<div><strong>Chip Model:</strong> " + String(ESP.getChipModel()) + "</div>";
  html += "<div><strong>Flash Size:</strong> " + String(ESP.getFlashChipSize()/1024/1024) + " MB</div>";
  html += "<div><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()/1024.0, 1) + " KB</div>";
  html += "</div>";

  // OTA Update Card
  html += "<div class='card'>";
  html += "<h3>Firmware Deployment</h3>";
  html += "<p>This is a factory setup tool. Use it to deploy the latest MMDVM firmware to this ESP32.</p>";
  html += "<br>";
  html += "<div><strong>Stable Version:</strong> <span id='latest-version'>Checking...</span></div>";
  html += "<div><strong>Beta Version:</strong> <span id='latest-beta-version'>Checking...</span></div>";
  html += "<br>";
  html += "<div class='info' style='background: #fff3cd; border-left-color: #ffc107; color: #000;'>";
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
  html += "<a href='javascript:void(0)' onclick='showFileUpload()' class='btn btn-primary'>Upload Custom Firmware</a>";
  html += "</div>";
  html += "<div id='upload-area' style='display:none; margin-top: 15px; padding: 15px; border: 2px dashed #007bff; border-radius: 6px; text-align: center;'>";
  html += "<input type='file' id='firmware-file' accept='.bin' style='margin: 10px 0;' />";
  html += "<br><button onclick='uploadFirmware()' class='btn btn-warning'>Upload Firmware</button>";
  html += "</div>";
  html += "<div id='update-status' style='margin-top: 10px; padding: 10px; display: none;'></div>";
  html += "</div>";

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

  // Upload functions
  html += "function showFileUpload() {";
  html += "  var uploadArea = document.getElementById('upload-area');";
  html += "  uploadArea.style.display = uploadArea.style.display === 'none' ? 'block' : 'none';";
  html += "}";

  html += "function uploadFirmware() {";
  html += "  var fileInput = document.getElementById('firmware-file');";
  html += "  var file = fileInput.files[0];";
  html += "  if (!file) { alert('Please select a firmware file (.bin)'); return; }";
  html += "  if (!file.name.endsWith('.bin')) { alert('Please select a valid .bin firmware file'); return; }";
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
  html += "    document.getElementById('update-status').innerHTML = '<div style=\"color: #28a745; font-size: 18px; font-weight: bold;\">Firmware flashed successfully!<br><br>Device is rebooting...<br><br>The device will restart with the full MMDVM firmware.<br>You may need to reconnect to your WiFi network.</div>';";
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
  html += "        var signalBars = signalStrength > -50 ? '📶' : signalStrength > -70 ? '📶' : '📶';";
  html += "        var lockIcon = network.encryption !== 0 ? '🔒' : '🔓';";
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
  html += "        statusDiv.innerHTML = '<div style=\"color: #28a745; padding: 10px; background: #d4edda; border-left: 4px solid #28a745; border-radius: 4px;\"><strong>✓ WiFi Connected!</strong><br>New IP Address: ' + newIP + '<br><br>Redirecting to new IP address in 5 seconds...<br>If not redirected, navigate to: <a href=\"http://' + newIP + '\" style=\"color: #007bff;\">http://' + newIP + '</a></div>';";
  html += "        setTimeout(() => { window.location.href = 'http://' + newIP; }, 5000);";
  html += "      } else {";
  html += "        statusDiv.innerHTML = '<div style=\"color: #dc3545; padding: 10px; background: #f8d7da; border-left: 4px solid #dc3545; border-radius: 4px;\"><strong>✗ Connection Failed</strong><br>' + data + '<br><br><button onclick=\"connectWiFi()\" style=\"padding: 8px 16px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer;\">Try Again</button></div>';";
  html += "      }";
  html += "    })";
  html += "    .catch(err => {";
  html += "      statusDiv.innerHTML = '<div style=\"color: #dc3545; padding: 10px; background: #f8d7da; border-left: 4px solid #dc3545; border-radius: 4px;\"><strong>✗ Connection Error</strong><br>Failed to communicate with device<br><br><button onclick=\"connectWiFi()\" style=\"padding: 8px 16px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer;\">Try Again</button></div>';";
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
  html += "window.onload = function() { initTheme(); checkLatestVersion(); checkLatestBetaVersion(); };";
  html += "</script>";

  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

#endif // WEBPAGES_H
