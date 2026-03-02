/*
 * System Network Page
 * Network, Ethernet, Time/NTP, and Web Security settings.
 */

#ifndef WEB_SYSTEM_NETWORK_H
#define WEB_SYSTEM_NETWORK_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

// --- Network ---
extern bool dnsFallbackEnabled;
extern String dnsFallbackIp;
extern String mdnsHostname;
extern bool mdnsEnabled;

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

// --- Time/NTP ---
extern int32_t ntpDaylightOffsetSec;
extern int32_t ntpGmtOffsetSec;
extern bool ntpEnabled;
extern String ntpServer;
extern uint32_t ntpSyncIntervalMs;

// --- Web Auth ---
extern bool webEnabled;
extern String webUsername;
extern String webPassword;
extern uint16_t webServerPort;

String getSystemNetworkPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Network Settings</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("system-network");

  html += "<div class='container'>";
  html += "<h1>Network Settings</h1>";
  html += "<p>Configure network discovery, Ethernet, time synchronisation, and web security</p>";

  html += "<div class='admin-grid'>";

  // Card 1: Network Settings
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
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>mDNS enables network discovery. DNS fallback is used if primary fails.</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveNetworkSettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetNetworkSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 2: Time Settings
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
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>NTP enables automatic time sync. GMT offset e.g. 3600 = GMT+1.</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveTimeSettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetTimeSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 3: Web Security
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
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>Web server port and credentials for web interface access.</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveSecuritySettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetSecuritySettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 4: Ethernet Settings
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
  html += "<summary style='cursor:pointer;color:var(--link-color);font-size:0.9em;'>Advanced Pin Configuration</summary>";
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
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>Configure Ethernet (W5500) SPI interface pins and settings.</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveEthSettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetEthSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";


  html += "</div>"; // close admin-grid

  // Save All & Reboot button
  html += "<div class='action-buttons-vertical' style='margin-top:20px;'>";
  html += "<button class='btn btn-success' onclick='saveAllNetworkSettings()'>Save All &amp; Reboot</button>";
  html += "</div>";
  html += "<div class='info' style='margin-top:20px'>";
  html += "<strong>Note:</strong> All network settings require a reboot to take effect. Use <b>Save All &amp; Reboot</b> to save all cards at once, or use the individual Save buttons on each card.";
  html += "</div>";

  // JavaScript
  html += "<script>";

  // Modal helpers
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
  html += "    var btns = document.createElement('div'); btns.className = 'modal-buttons';";
  html += "    var ok = document.createElement('button'); ok.textContent = 'OK'; ok.className = 'btn btn-primary'; ok.onclick = close;";
  html += "    btns.appendChild(ok); box.appendChild(btns);";
  html += "  });";
  html += "};";
  html += "window.showConfirm = function(msg, onYes) {";
  html += "  showModal(function(box, close) {";
  html += "    box.innerHTML = '<h4>' + msg + '</h4>';";
  html += "    var btns = document.createElement('div'); btns.className = 'modal-buttons';";
  html += "    var yes = document.createElement('button'); yes.textContent = 'Yes'; yes.className = 'btn btn-success';";
  html += "    yes.onclick = function() { close(); onYes(); };";
  html += "    var no = document.createElement('button'); no.textContent = 'Cancel'; no.className = 'btn btn-danger'; no.onclick = close;";
  html += "    btns.appendChild(yes); btns.appendChild(no); box.appendChild(btns);";
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
  html += "  var mdnsEn = document.getElementById('mdns-enabled').checked ? '1' : '0';";
  html += "  var hostname = document.getElementById('hostname').value;";
  html += "  var dnsFallback = document.getElementById('dns-fallback-enabled').checked ? '1' : '0';";
  html += "  var dnsFallbackIp = document.getElementById('dns-fallback-ip').value;";
  html += "  if (!hostname || hostname.length < 3) { showAlert('Hostname must be at least 3 characters'); return; }";
  html += "  showConfirm('Save network settings and reboot?', function() {";
  html += "    fetch('/api/save-network-settings', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'mdns=' + mdnsEn + '&hostname=' + encodeURIComponent(hostname) + '&dnsfallback=' + dnsFallback + '&dnsfallbackip=' + encodeURIComponent(dnsFallbackIp)";
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

  // Ethernet Settings
  html += "function saveEthSettings() {";
  html += "  var ethEn = document.getElementById('eth-enabled').checked ? '1' : '0';";
  html += "  var ethDbg = document.getElementById('eth-debug').checked ? '1' : '0';";
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
  html += "      body: 'ethenabled=' + ethEn + '&ethdebug=' + ethDbg + '&ethmiso=' + ethMiso + '&ethmosi=' + ethMosi + '&ethsclk=' + ethSclk + '&ethcs=' + ethCs + '&ethint=' + ethInt + '&ethrst=' + ethRst + '&ethaddr=' + ethAddr + '&ethconntimeout=' + ethConnTimeout";
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
  html += "  var ntpEn = document.getElementById('ntp-enabled').checked ? '1' : '0';";
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
  html += "      body: 'ntpenabled=' + ntpEn + '&server=' + encodeURIComponent(server) + '&gmt=' + gmt + '&dst=' + dst + '&sync=' + sync";
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

  // Web Security
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
  html += "function saveAllNetworkSettings() {";
  html += "  var hostname = document.getElementById('hostname').value;";
  html += "  var server = document.getElementById('ntp-server').value;";
  html += "  var sync = document.getElementById('ntp-sync').value;";
  html += "  var webPort = document.getElementById('web-port').value;";
  html += "  var username = document.getElementById('web-username').value;";
  html += "  var password = document.getElementById('web-password').value;";
  html += "  if (!hostname || hostname.length < 3) { showAlert('Hostname must be at least 3 characters'); return; }";
  html += "  if (!server || server.length < 3) { showAlert('NTP server must be at least 3 characters'); return; }";
  html += "  if (sync < 60000 || sync > 86400000) { showAlert('Sync interval must be 60000-86400000 ms'); return; }";
  html += "  if (webPort < 1 || webPort > 65535) { showAlert('Web port must be 1-65535'); return; }";
  html += "  if (!username || username.length < 1) { showAlert('Username cannot be empty'); return; }";
  html += "  if (!password || password.length < 1) { showAlert('Password cannot be empty'); return; }";
  html += "  showConfirm('Save all network settings and reboot?', function() {";
  html += "    var post = function(url, body) { return fetch(url, {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:body}); };";
  html += "    var mdnsEn = document.getElementById('mdns-enabled').checked ? '1' : '0';";
  html += "    var dnsFb = document.getElementById('dns-fallback-enabled').checked ? '1' : '0';";
  html += "    var dnsFbIp = document.getElementById('dns-fallback-ip').value;";
  html += "    var ethEn = document.getElementById('eth-enabled').checked ? '1' : '0';";
  html += "    var ethDbg = document.getElementById('eth-debug').checked ? '1' : '0';";
  html += "    var eMiso = document.getElementById('eth-miso').value;";
  html += "    var eMosi = document.getElementById('eth-mosi').value;";
  html += "    var eSclk = document.getElementById('eth-sclk').value;";
  html += "    var eCs = document.getElementById('eth-cs').value;";
  html += "    var eInt = document.getElementById('eth-int').value;";
  html += "    var eRst = document.getElementById('eth-rst').value;";
  html += "    var eAddr = document.getElementById('eth-addr').value;";
  html += "    var eCto = document.getElementById('eth-conn-timeout').value;";
  html += "    var ntpEn = document.getElementById('ntp-enabled').checked ? '1' : '0';";
  html += "    var gmt = document.getElementById('ntp-gmt').value;";
  html += "    var dst = document.getElementById('ntp-dst').value;";
  html += "    var webAuthEn = document.getElementById('web-auth-enabled').checked ? '1' : '0';";
  html += "    post('/api/save-network-settings', 'mdns=' + mdnsEn + '&hostname=' + encodeURIComponent(hostname) + '&dnsfallback=' + dnsFb + '&dnsfallbackip=' + encodeURIComponent(dnsFbIp))";
  html += "    .then(function() { return post('/api/save-eth-settings', 'ethenabled=' + ethEn + '&ethdebug=' + ethDbg + '&ethmiso=' + eMiso + '&ethmosi=' + eMosi + '&ethsclk=' + eSclk + '&ethcs=' + eCs + '&ethint=' + eInt + '&ethrst=' + eRst + '&ethaddr=' + eAddr + '&ethconntimeout=' + eCto); })";
  html += "    .then(function() { return post('/api/save-time-settings', 'ntpenabled=' + ntpEn + '&server=' + encodeURIComponent(server) + '&gmt=' + gmt + '&dst=' + dst + '&sync=' + sync); })";
  html += "    .then(function() { return post('/api/save-security-settings', 'webenabled=' + webAuthEn + '&port=' + webPort + '&username=' + encodeURIComponent(username) + '&password=' + encodeURIComponent(password)); })";
  html += "    .then(function() {";
  html += "      showAlert('All network settings saved.<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    }).catch(function(err) { showAlert('Error saving settings: ' + err); });";
  html += "  });";
  html += "}";

  html += "</script>";

  html += "</div>"; // close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SYSTEM_NETWORK_H
