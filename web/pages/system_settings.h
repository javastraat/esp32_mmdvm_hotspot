/*
 * System Settings Page
 * General system settings and preferences
 */

#ifndef WEB_SYSTEM_SETTINGS_H
#define WEB_SYSTEM_SETTINGS_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

// External references to runtime settings
// --- Ethernet ---
extern int ethAddr;
extern int ethConnectTimeout;
extern int ethCsPin;
extern bool ethDebug;
extern bool ethEnabled;
extern int ethIntPin;
extern int ethMisoPin;
extern int ethMosiPin;
extern int ethRstPin;
extern int ethSclkPin;

// --- LED/Button ---
extern int buttonPin;
extern int ledPin;

// --- Network ---
extern bool dnsFallbackEnabled;
extern String dnsFallbackIp;
extern String mdnsHostname;
extern bool mdnsEnabled;
extern uint16_t webServerPort;

// --- OLED ---
extern int i2cSclPin;
extern int i2cSdaPin;
extern int oledHeight;
extern int oledI2cAddress;
extern bool oledEnabled;
extern int oledWidth;

// --- Time/NTP ---
extern int32_t ntpDaylightOffsetSec;
extern int32_t ntpGmtOffsetSec;
extern bool ntpEnabled;
extern String ntpServer;
extern uint32_t ntpSyncIntervalMs;

// --- SD Card ---
extern bool sdcardEnabled;
extern int sdCsPin;
extern int spiMisoPin;
extern int spiMosiPin;
extern int spiSclkPin;

// --- Web Auth ---
extern bool webEnabled;
extern String webUsername;
extern String webPassword;

// --- WiFi ---
extern String wifiApPassword;
extern uint8_t wifiApChannel;
extern String wifiApSsid;
extern uint8_t wifiMaxRetries;

String getSystemSettingsPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>System Settings</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("system-settings");

  html += "<div class='container'>";
  html += "<h1>System Settings</h1>";
  html += "<p>Configure network, time, and system preferences</p>";

  html += "<div class='admin-grid'>";

  // Card 1: Network Settings (styled like ArduinoOTA card)
  html += "<div class='card'>";
  html += "<h3>Network Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>mDNS Enabled:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='mdns-enabled' " + String(mdnsEnabled ? "checked" : "") + ">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>mDNS Hostname:</span>";
  html += "<input type='text' id='hostname' value='" + mdnsHostname + "' maxlength='32' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>DNS Fallback Enabled:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='dns-fallback-enabled'" + String(dnsFallbackEnabled ? " checked" : "") + ">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Fallback DNS IP:</span>";
  html += "<input type='text' id='dns-fallback-ip' value='" + dnsFallbackIp + "' placeholder='8.8.8.8' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "mDNS enables network discovery. DNS fallback is used if primary fails.";
  html += "</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveNetworkSettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetNetworkSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "<div id='network-settings-status' style='margin-top: 10px; padding: 10px; display: none;'></div>";
  html += "</div>";

  // Card 2: WiFi AP Settings (styled like ArduinoOTA card)
  html += "<div class='card'>";
  html += "<h3>WiFi Access Point</h3>";
  html += "<form onsubmit=\"return false;\">";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>AP SSID:</span>";
  html += "<input type='text' id='wifi-ap-ssid' value='" + wifiApSsid + "' maxlength='32' style='width: 120px; padding-right: 8px;' autocomplete='username'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>AP Password:</span>";
  html += "<input type='password' id='wifi-ap-password' value='" + wifiApPassword + "' minlength='8' maxlength='63' style='width: 104px; padding-right: 24px;' autocomplete='current-password'>";
  html += "<span id='wifi-ap-password-eye' style=\"position:absolute; right:8px; top:50%; transform:translateY(-50%); cursor:pointer;\" onclick=\"togglePasswordVisibility('wifi-ap-password', this)\"><svg width=18 height=18 viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><path d='M1 12s4-7 11-7 11 7 11 7-4 7-11 7-11-7-11-7z'/><circle cx='12' cy='12' r='3'/></svg></span>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>AP Channel:</span>";
  html += "<input type='text' id='wifi-ap-channel' value='" + String(wifiApChannel) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Max Retries:</span>";
  html += "<input type='text' id='wifi-max-retries' value='" + String(wifiMaxRetries) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "WiFi AP is used as fallback when connection fails. Password must be at least 8 characters.";
  html += "</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' type='button' onclick='saveWifiApSettings()'>Save</button>";
  html += "<button class='btn btn-danger' type='button' onclick='resetWifiApSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "<div id='wifi-ap-settings-status' style='margin-top: 10px; padding: 10px; display: none;'></div>";
  html += "</form>";
  html += "</div>";

  // Card 3: Ethernet Settings
  html += "<div class='card'>";
  html += "<h3>Ethernet Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Ethernet Enabled:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='eth-enabled'" + String(ethEnabled ? " checked" : "") + ">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Ethernet Debug:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='eth-debug'" + String(ethDebug ? " checked" : "") + ">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<details style='margin-top:10px;'>";
  html += "<summary style='cursor:pointer;color:#007bff;font-size:0.9em;'>Advanced Pin Configuration</summary>";
  html += "<div style='margin-top:8px;'>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>ETH MISO Pin:</span>";
  html += "<input type='text' id='eth-miso' value='" + String(ethMisoPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>ETH MOSI Pin:</span>";
  html += "<input type='text' id='eth-mosi' value='" + String(ethMosiPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>ETH SCLK Pin:</span>";
  html += "<input type='text' id='eth-sclk' value='" + String(ethSclkPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>ETH CS Pin:</span>";
  html += "<input type='text' id='eth-cs' value='" + String(ethCsPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>ETH INT Pin:</span>";
  html += "<input type='text' id='eth-int' value='" + String(ethIntPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>ETH RST Pin:</span>";
  html += "<input type='text' id='eth-rst' value='" + String(ethRstPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>ETH Address:</span>";
  html += "<input type='text' id='eth-addr' value='" + String(ethAddr) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Connect Timeout:</span>";
  html += "<input type='text' id='eth-conn-timeout' value='" + String(ethConnectTimeout) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "</div>";
  html += "</details>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "Configure Ethernet (W5500) SPI interface pins and settings.";
  html += "</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveEthSettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetEthSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 4: LED & Button Settings
  html += "<div class='card'>";
  html += "<h3>LED & Button Settings</h3>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>LED Pin:</span>";
  html += "<input type='text' id='led-pin' value='" + String(ledPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Button Pin:</span>";
  html += "<input type='text' id='button-pin' value='" + String(buttonPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "GPIO pin for built-in LED and OLED toggle button.";
  html += "</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveLedButtonSettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetLedButtonSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 5: OLED Settings
  html += "<div class='card'>";
  html += "<h3>OLED Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>OLED Enabled:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='oled-enable'" + String(oledEnabled ? " checked" : "") + ">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<details style='margin-top:10px;'>";
  html += "<summary style='cursor:pointer;color:#007bff;font-size:0.9em;'>Advanced Pin Configuration</summary>";
  html += "<div style='margin-top:8px;'>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>I2C SDA Pin:</span>";
  html += "<input type='text' id='i2c-sda' value='" + String(i2cSdaPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>I2C SCL Pin:</span>";
  html += "<input type='text' id='i2c-scl' value='" + String(i2cSclPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>I2C Address:</span>";
  {
    String hexAddr = "0x" + String(oledI2cAddress, HEX);
    hexAddr.toUpperCase();
    html += "<input type='text' id='oled-addr' value='" + (oledI2cAddress == 0x3C ? "0x3C" : (oledI2cAddress == 0x3D ? "0x3D" : hexAddr)) + "' style='width: 120px; padding-right: 8px;'>";
  }
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>OLED Width:</span>";
  html += "<input type='text' id='oled-width' value='" + String(oledWidth) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>OLED Height:</span>";
  html += "<input type='text' id='oled-height' value='" + String(oledHeight) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "</div>";
  html += "</details>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "Configure OLED display I2C pins, address, and resolution.";
  html += "</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveOledSettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetOledSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 6: Time Settings
  html += "<div class='card'>";
  html += "<h3>Time Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>NTP Enabled:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='ntp-enabled'" + String(ntpEnabled ? " checked" : "") + ">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>NTP Server:</span>";
  html += "<input type='text' id='ntp-server' value='" + ntpServer + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>GMT Offset (s):</span>";
  html += "<input type='text' id='ntp-gmt' value='" + String(ntpGmtOffsetSec) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>DST Offset (s):</span>";
  html += "<input type='text' id='ntp-dst' value='" + String(ntpDaylightOffsetSec) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Sync Interval (ms):</span>";
  html += "<input type='text' id='ntp-sync' value='" + String(ntpSyncIntervalMs) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "NTP enables automatic time sync. GMT offset e.g. 3600 = GMT+1.";
  html += "</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveTimeSettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetTimeSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 7: SD Card Settings
  html += "<div class='card'>";
  html += "<h3>SD Card Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>SD Card Enabled:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='sdcard-enabled'" + String(sdcardEnabled ? " checked" : "") + ">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<details style='margin-top:10px;'>";
  html += "<summary style='cursor:pointer;color:#007bff;font-size:0.9em;'>Advanced Pin Configuration</summary>";
  html += "<div style='margin-top:8px;'>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>SPI MISO Pin:</span>";
  html += "<input type='text' id='spi-miso' value='" + String(spiMisoPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>SPI MOSI Pin:</span>";
  html += "<input type='text' id='spi-mosi' value='" + String(spiMosiPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>SPI SCLK Pin:</span>";
  html += "<input type='text' id='spi-sclk' value='" + String(spiSclkPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>SD CS Pin:</span>";
  html += "<input type='text' id='sd-cs' value='" + String(sdCsPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "</div>";
  html += "</details>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "Configure SD card SPI interface pins.";
  html += "</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveSdCardSettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetSdCardSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 8: Web Security
  html += "<div class='card'>";
  html += "<h3>Web Security</h3>";
  html += "<p>Authentication and access control</p>";
  html += "<form onsubmit='return false;'>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Web Auth Enabled:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='web-auth-enabled'" + String(webEnabled ? " checked" : "") + ">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Web Port:</span>";
  html += "<input type='text' id='web-port' value='" + String(webServerPort) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Username:</span>";
  html += "<input type='text' id='web-username' value='" + webUsername + "' maxlength='32' style='width: 120px; padding-right: 8px;' autocomplete='username'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Password:</span>";
  html += "<input type='password' id='web-password' value='" + webPassword + "' maxlength='64' style='width: 104px; padding-right: 24px;' autocomplete='current-password'>";
  html += "<span id='web-password-eye' style=\"position:absolute; right:8px; top:50%; transform:translateY(-50%); cursor:pointer;\" onclick=\"togglePasswordVisibility('web-password', this)\"><svg width=18 height=18 viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><path d='M1 12s4-7 11-7 11 7 11 7-4 7-11 7-11-7-11-7z'/><circle cx='12' cy='12' r='3'/></svg></span>";
  html += "</div>";
  html += "</form>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "Web server port and credentials for web interface access.";
  html += "</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveSecuritySettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetSecuritySettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  // Save All & Reboot button
  html += "<div class='action-buttons-vertical' style='margin-top:20px;'>";
  html += "<button class='btn btn-success' onclick='saveAllSettings()'>Save All & Reboot</button>";
  html += "</div>";
  html += "<div class='info' style='margin-top:20px'>";
  html += "<strong>Note:</strong> After changing settings, click Save All & Reboot for all changes to take effect.";
  html += "</div>";

  // JavaScript functions
  html += "<script>";

  // Styled modal helpers (matching system_wifi / system_admin)
  html += "window.showModal = function(contentFn) {";
  html += "  var overlay = document.createElement('div');";
  html += "  overlay.className = 'modal-overlay';";
  html += "  var box = document.createElement('div');";
  html += "  box.className = 'modal-box';";
  html += "  contentFn(box, function() { document.body.removeChild(overlay); });";
  html += "  overlay.appendChild(box);";
  html += "  overlay.addEventListener('click', function(e) { if (e.target === overlay) document.body.removeChild(overlay); });";
  html += "  document.body.appendChild(overlay);";
  html += "  return overlay;";
  html += "};";
  html += "window.showAlert = function(msg) {";
  html += "  showModal(function(box, close) {";
  html += "    box.innerHTML = '<h4>' + msg + '</h4>';";
  html += "    var btns = document.createElement('div');";
  html += "    btns.className = 'modal-buttons';";
  html += "    var ok = document.createElement('button');";
  html += "    ok.textContent = 'OK';";
  html += "    ok.className = 'btn btn-primary';";
  html += "    ok.onclick = close;";
  html += "    btns.appendChild(ok);";
  html += "    box.appendChild(btns);";
  html += "  });";
  html += "};";
  html += "window.showConfirm = function(msg, onYes) {";
  html += "  showModal(function(box, close) {";
  html += "    box.innerHTML = '<h4>' + msg + '</h4>';";
  html += "    var btns = document.createElement('div');";
  html += "    btns.className = 'modal-buttons';";
  html += "    var yes = document.createElement('button');";
  html += "    yes.textContent = 'Yes';";
  html += "    yes.className = 'btn btn-success';";
  html += "    yes.onclick = function() { close(); onYes(); };";
  html += "    var no = document.createElement('button');";
  html += "    no.textContent = 'Cancel';";
  html += "    no.className = 'btn btn-danger';";
  html += "    no.onclick = close;";
  html += "    btns.appendChild(yes);";
  html += "    btns.appendChild(no);";
  html += "    box.appendChild(btns);";
  html += "  });";
  html += "};";

  // Password toggle
  html += "function togglePasswordVisibility(id, icon) {";
  html += "  var input = document.getElementById(id);";
  html += "  if (input.type === 'password') {";
  html += "    input.type = 'text';";
  html += "    icon.innerHTML = \"<svg width=18 height=18 viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><path d='M1 12s4-7 11-7 11 7 11 7-4 7-11 7-11-7-11-7z'/><circle cx='12' cy='12' r='3'/></svg>\";";
  html += "  } else {";
  html += "    input.type = 'password';";
  html += "    icon.innerHTML = \"<svg width=18 height=18 viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><path d='M1 12s4-7 11-7 11 7 11 7-4 7-11 7-11-7-11-7z'/><circle cx='12' cy='12' r='3'/></svg>\";";
  html += "  }";
  html += "}";

  // Network Settings
  html += "function saveNetworkSettings() {";
  html += "  var mdnsEnabled = document.getElementById('mdns-enabled').checked ? '1' : '0';";
  html += "  var hostname = document.getElementById('hostname').value;";
  html += "  var dnsFallback = document.getElementById('dns-fallback-enabled').checked ? '1' : '0';";
  html += "  var dnsFallbackIp = document.getElementById('dns-fallback-ip').value;";
  html += "  if (!hostname || hostname.length < 3) { showAlert('Hostname must be at least 3 characters'); return; }";
  html += "  showConfirm('Save network settings and reboot?', function() {";
  html += "    fetch('/api/save-network-settings', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'mdns=' + mdnsEnabled + '&hostname=' + encodeURIComponent(hostname) + '&dnsfallback=' + dnsFallback + '&dnsfallbackip=' + encodeURIComponent(dnsFallbackIp)";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  html += "function resetNetworkSettings() {";
  html += "  showConfirm('Reset network settings to default and reboot?', function() {";
  html += "    fetch('/api/reset-network-settings', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  // WiFi AP Settings
  html += "function saveWifiApSettings() {";
  html += "  var ssid = document.getElementById('wifi-ap-ssid').value;";
  html += "  var password = document.getElementById('wifi-ap-password').value;";
  html += "  var channel = document.getElementById('wifi-ap-channel').value;";
  html += "  var retries = document.getElementById('wifi-max-retries').value;";
  html += "  if (!ssid || ssid.length < 3) { showAlert('WiFi AP SSID must be at least 3 characters'); return; }";
  html += "  if (!password || password.length < 8) { showAlert('WiFi AP password must be at least 8 characters'); return; }";
  html += "  if (channel < 1 || channel > 13) { showAlert('WiFi AP channel must be 1-13'); return; }";
  html += "  if (retries < 1 || retries > 20) { showAlert('Max retries must be 1-20'); return; }";
  html += "  showConfirm('Save WiFi AP settings and reboot?', function() {";
  html += "    fetch('/api/save-wifi-ap-settings', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password) + '&channel=' + channel + '&retries=' + retries";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  html += "function resetWifiApSettings() {";
  html += "  showConfirm('Reset WiFi AP settings to default and reboot?', function() {";
  html += "    fetch('/api/reset-wifi-ap-settings', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  // Ethernet Settings
  html += "function saveEthSettings() {";
  html += "  var ethEnabled = document.getElementById('eth-enabled').checked ? '1' : '0';";
  html += "  var ethDebug = document.getElementById('eth-debug').checked ? '1' : '0';";
  html += "  var ethMiso = document.getElementById('eth-miso').value;";
  html += "  var ethMosi = document.getElementById('eth-mosi').value;";
  html += "  var ethSclk = document.getElementById('eth-sclk').value;";
  html += "  var ethCs = document.getElementById('eth-cs').value;";
  html += "  var ethInt = document.getElementById('eth-int').value;";
  html += "  var ethRst = document.getElementById('eth-rst').value;";
  html += "  var ethAddr = document.getElementById('eth-addr').value;";
  html += "  var ethConnTimeout = document.getElementById('eth-conn-timeout').value;";
  html += "  showConfirm('Save Ethernet settings and reboot?', function() {";
  html += "    fetch('/api/save-eth-settings', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'ethenabled=' + ethEnabled + '&ethdebug=' + ethDebug + '&ethmiso=' + ethMiso + '&ethmosi=' + ethMosi + '&ethsclk=' + ethSclk + '&ethcs=' + ethCs + '&ethint=' + ethInt + '&ethrst=' + ethRst + '&ethaddr=' + ethAddr + '&ethconntimeout=' + ethConnTimeout";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  html += "function resetEthSettings() {";
  html += "  showConfirm('Reset Ethernet settings to default and reboot?', function() {";
  html += "    fetch('/api/reset-eth-settings', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  // Time Settings
  html += "function saveTimeSettings() {";
  html += "  var ntpEnabled = document.getElementById('ntp-enabled').checked ? '1' : '0';";
  html += "  var server = document.getElementById('ntp-server').value;";
  html += "  var gmt = document.getElementById('ntp-gmt').value;";
  html += "  var dst = document.getElementById('ntp-dst').value;";
  html += "  var sync = document.getElementById('ntp-sync').value;";
  html += "  if (!server || server.length < 3) { showAlert('NTP server must be at least 3 characters'); return; }";
  html += "  if (sync < 60000 || sync > 86400000) { showAlert('Sync interval must be 60000-86400000 ms'); return; }";
  html += "  showConfirm('Save time settings and reboot?', function() {";
  html += "    fetch('/api/save-time-settings', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'ntpenabled=' + ntpEnabled + '&server=' + encodeURIComponent(server) + '&gmt=' + gmt + '&dst=' + dst + '&sync=' + sync";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  html += "function resetTimeSettings() {";
  html += "  showConfirm('Reset time settings to default and reboot?', function() {";
  html += "    fetch('/api/reset-time-settings', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  // LED/Button Settings
  html += "function saveLedButtonSettings() {";
  html += "  var ledPin = document.getElementById('led-pin').value;";
  html += "  var buttonPin = document.getElementById('button-pin').value;";
  html += "  if (ledPin < 0 || ledPin > 48) { showAlert('LED pin must be 0-48'); return; }";
  html += "  if (buttonPin < 0 || buttonPin > 48) { showAlert('Button pin must be 0-48'); return; }";
  html += "  showConfirm('Save LED/Button settings and reboot?', function() {";
  html += "    fetch('/api/save-led-button-settings', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'ledpin=' + ledPin + '&buttonpin=' + buttonPin";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  html += "function resetLedButtonSettings() {";
  html += "  showConfirm('Reset LED/Button settings to default and reboot?', function() {";
  html += "    fetch('/api/reset-led-button-settings', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  // SD Card Settings
  html += "function saveSdCardSettings() {";
  html += "  var sdEnabled = document.getElementById('sdcard-enabled').checked ? '1' : '0';";
  html += "  var spiMiso = document.getElementById('spi-miso').value;";
  html += "  var spiMosi = document.getElementById('spi-mosi').value;";
  html += "  var spiSclk = document.getElementById('spi-sclk').value;";
  html += "  var sdCs = document.getElementById('sd-cs').value;";
  html += "  showConfirm('Save SD Card settings and reboot?', function() {";
  html += "    fetch('/api/save-sdcard-settings', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'sdcard_en=' + sdEnabled + '&spi_miso=' + spiMiso + '&spi_mosi=' + spiMosi + '&spi_sclk=' + spiSclk + '&sd_cs=' + sdCs";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  html += "function resetSdCardSettings() {";
  html += "  showConfirm('Reset SD Card settings to default and reboot?', function() {";
  html += "    fetch('/api/reset-sdcard-settings', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  // OLED Settings
  html += "function saveOledSettings() {";
  html += "  var oledEnabled = document.getElementById('oled-enable').checked ? '1' : '0';";
  html += "  var i2cSda = document.getElementById('i2c-sda').value;";
  html += "  var i2cScl = document.getElementById('i2c-scl').value;";
  html += "  var oledAddr = document.getElementById('oled-addr').value;";
  html += "  var oledWidth = document.getElementById('oled-width').value;";
  html += "  var oledHeight = document.getElementById('oled-height').value;";
  html += "  if (i2cSda < 0 || i2cSda > 48) { showAlert('I2C SDA pin must be 0-48'); return; }";
  html += "  if (i2cScl < 0 || i2cScl > 48) { showAlert('I2C SCL pin must be 0-48'); return; }";
  html += "  var addrUpper = oledAddr.toUpperCase(); if (addrUpper != '0X3C' && addrUpper != '0X3D') { showAlert('OLED I2C address must be 0x3C or 0x3D'); return; }";
  html += "  if (oledWidth < 64 || oledWidth > 256) { showAlert('OLED width must be 64-256'); return; }";
  html += "  if (oledHeight < 32 || oledHeight > 128) { showAlert('OLED height must be 32-128'); return; }";
  html += "  showConfirm('Save OLED settings and reboot?', function() {";
  html += "    fetch('/api/save-oled-settings', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'oledenabled=' + oledEnabled + '&i2csda=' + i2cSda + '&i2cscl=' + i2cScl + '&oledaddr=' + oledAddr + '&oledwidth=' + oledWidth + '&oledheight=' + oledHeight";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  html += "function resetOledSettings() {";
  html += "  showConfirm('Reset OLED settings to default and reboot?', function() {";
  html += "    fetch('/api/reset-oled-settings', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  // Security Settings
  html += "function saveSecuritySettings() {";
  html += "  var webAuthEn = document.getElementById('web-auth-enabled').checked ? '1' : '0';";
  html += "  var webPort = document.getElementById('web-port').value;";
  html += "  var username = document.getElementById('web-username').value;";
  html += "  var password = document.getElementById('web-password').value;";
  html += "  if (webPort < 1 || webPort > 65535) { showAlert('Web port must be 1-65535'); return; }";
  html += "  if (!username || username.length < 1) { showAlert('Username cannot be empty'); return; }";
  html += "  if (!password || password.length < 1) { showAlert('Password cannot be empty'); return; }";
  html += "  showConfirm('Save security settings and reboot?', function() {";
  html += "    fetch('/api/save-security-settings', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'webenabled=' + webAuthEn + '&port=' + webPort + '&username=' + encodeURIComponent(username) + '&password=' + encodeURIComponent(password)";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  html += "function resetSecuritySettings() {";
  html += "  showConfirm('Reset security settings to default and reboot?', function() {";
  html += "    fetch('/api/reset-security-settings', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  // Save All & Reboot
  html += "function saveAllSettings() {";
  html += "  showConfirm('Save all settings and reboot?', function() {";
  html += "  var post = function(url, body) {";
  html += "    return fetch(url, { method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: body });";
  html += "  };";
  // Network
  html += "  var mdnsEn = document.getElementById('mdns-enabled').checked ? '1' : '0';";
  html += "  var hostname = document.getElementById('hostname').value;";
  html += "  var dnsFb = document.getElementById('dns-fallback-enabled').checked ? '1' : '0';";
  html += "  var dnsFbIp = document.getElementById('dns-fallback-ip').value;";
  // WiFi AP
  html += "  var apSsid = document.getElementById('wifi-ap-ssid').value;";
  html += "  var apPass = document.getElementById('wifi-ap-password').value;";
  html += "  var apCh = document.getElementById('wifi-ap-channel').value;";
  html += "  var apRet = document.getElementById('wifi-max-retries').value;";
  // Ethernet
  html += "  var ethEn = document.getElementById('eth-enabled').checked ? '1' : '0';";
  html += "  var ethDbg = document.getElementById('eth-debug').checked ? '1' : '0';";
  html += "  var eMiso = document.getElementById('eth-miso').value;";
  html += "  var eMosi = document.getElementById('eth-mosi').value;";
  html += "  var eSclk = document.getElementById('eth-sclk').value;";
  html += "  var eCs = document.getElementById('eth-cs').value;";
  html += "  var eInt = document.getElementById('eth-int').value;";
  html += "  var eRst = document.getElementById('eth-rst').value;";
  html += "  var eAddr = document.getElementById('eth-addr').value;";
  html += "  var eCto = document.getElementById('eth-conn-timeout').value;";
  // LED/Button
  html += "  var lPin = document.getElementById('led-pin').value;";
  html += "  var bPin = document.getElementById('button-pin').value;";
  // OLED
  html += "  var oledEn = document.getElementById('oled-enable').checked ? '1' : '0';";
  html += "  var sda = document.getElementById('i2c-sda').value;";
  html += "  var scl = document.getElementById('i2c-scl').value;";
  html += "  var oAddr = document.getElementById('oled-addr').value;";
  html += "  var oW = document.getElementById('oled-width').value;";
  html += "  var oH = document.getElementById('oled-height').value;";
  // Time
  html += "  var ntpEn = document.getElementById('ntp-enabled').checked ? '1' : '0';";
  html += "  var ntpSrv = document.getElementById('ntp-server').value;";
  html += "  var ntpGmt = document.getElementById('ntp-gmt').value;";
  html += "  var ntpDst = document.getElementById('ntp-dst').value;";
  html += "  var ntpSync = document.getElementById('ntp-sync').value;";
  // SD Card
  html += "  var sdEn = document.getElementById('sdcard-enabled').checked ? '1' : '0';";
  html += "  var sMiso = document.getElementById('spi-miso').value;";
  html += "  var sMosi = document.getElementById('spi-mosi').value;";
  html += "  var sSclk = document.getElementById('spi-sclk').value;";
  html += "  var sCsPin = document.getElementById('sd-cs').value;";
  // Security
  html += "  var wAuthEn = document.getElementById('web-auth-enabled').checked ? '1' : '0';";
  html += "  var wPort = document.getElementById('web-port').value;";
  html += "  var wUser = document.getElementById('web-username').value;";
  html += "  var wPass = document.getElementById('web-password').value;";
  // Send all sequentially
  html += "  post('/api/save-network-settings', 'mdns=' + mdnsEn + '&hostname=' + encodeURIComponent(hostname) + '&dnsfallback=' + dnsFb + '&dnsfallbackip=' + encodeURIComponent(dnsFbIp))";
  html += "  .then(function() { return post('/api/save-wifi-ap-settings', 'ssid=' + encodeURIComponent(apSsid) + '&password=' + encodeURIComponent(apPass) + '&channel=' + apCh + '&retries=' + apRet); })";
  html += "  .then(function() { return post('/api/save-eth-settings', 'ethenabled=' + ethEn + '&ethdebug=' + ethDbg + '&ethmiso=' + eMiso + '&ethmosi=' + eMosi + '&ethsclk=' + eSclk + '&ethcs=' + eCs + '&ethint=' + eInt + '&ethrst=' + eRst + '&ethaddr=' + eAddr + '&ethconntimeout=' + eCto); })";
  html += "  .then(function() { return post('/api/save-led-button-settings', 'ledpin=' + lPin + '&buttonpin=' + bPin); })";
  html += "  .then(function() { return post('/api/save-oled-settings', 'oledenabled=' + oledEn + '&i2csda=' + sda + '&i2cscl=' + scl + '&oledaddr=' + oAddr + '&oledwidth=' + oW + '&oledheight=' + oH); })";
  html += "  .then(function() { return post('/api/save-time-settings', 'ntpenabled=' + ntpEn + '&server=' + encodeURIComponent(ntpSrv) + '&gmt=' + ntpGmt + '&dst=' + ntpDst + '&sync=' + ntpSync); })";
  html += "  .then(function() { return post('/api/save-sdcard-settings', 'sdcard_en=' + sdEn + '&spi_miso=' + sMiso + '&spi_mosi=' + sMosi + '&spi_sclk=' + sSclk + '&sd_cs=' + sCsPin); })";
  html += "  .then(function() { return post('/api/save-security-settings', 'webenabled=' + wAuthEn + '&port=' + wPort + '&username=' + encodeURIComponent(wUser) + '&password=' + encodeURIComponent(wPass)); })";
  html += "  .then(function() {";
  html += "    showAlert('All settings saved.<br><br>The device will now reboot.');";
  html += "    fetch('/api/reboot', {method: 'POST'});";
  html += "    document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "    setTimeout(function() { location.reload(); }, 10000);";
  html += "  }).catch(function(err) {";
  html += "    showAlert('Error saving settings: ' + err);";
  html += "  });";
  html += "  });";
  html += "}";

  html += "</script>";

  html += "</div>"; // Close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SYSTEM_SETTINGS_H
