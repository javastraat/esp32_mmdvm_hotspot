/*
 * System WireGuard Page
 * WireGuard VPN configuration and status
 */

#ifndef WEB_SYSTEM_WIREGUARD_H
#define WEB_SYSTEM_WIREGUARD_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

// External references to runtime WireGuard settings
extern bool wgEnabled;
extern String wgLocalIp;
extern String wgPrivateKey;
extern String wgPublicKey;
extern String wgEndpoint;
extern String wgDns;
extern String wgAllowedIps;
extern uint16_t wgEndpointPort;
extern bool wireguardConnected;

String getSystemWireguardPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>WireGuard VPN</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("system-wireguard");

  html += "<div class='container'>";
  html += "<h1>WireGuard VPN</h1>";
  html += "<p>Configure WireGuard VPN tunnel to securely connect this hotspot to your network</p>";

  html += "<div class='admin-grid'>";

  // Card 1: WireGuard Status & Enable
  html += "<div class='card'>";
  html += "<h3>WireGuard Service</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enable WireGuard:</span>";
  html += "<label class='switch'><input type='checkbox' id='wg-enabled'" + String(wgEnabled ? " checked" : "") + "><span class='slider'></span></label>";
  html += "</div>";
  html += "<div class='metric'><span class='metric-label'>Status:</span><span class='metric-value'>";
  if (!wgEnabled)
    html += "Disabled";
  else if (wireguardConnected)
    html += "<span style='color:var(--success-color,#28a745);'>Connected</span>";
  else
    html += "<span style='color:var(--warning-color,#ffc107);'>Connecting...</span>";
  html += "</span></div>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveWgService()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetWgService()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 2: WireGuard Configuration
  html += "<div class='card'>";
  html += "<h3>Tunnel Configuration</h3>";
  html += "<form onsubmit=\"return false;\">";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Local IP:</span>";
  html += "<input type='text' id='wg-local-ip' value='" + wgLocalIp + "' placeholder='10.0.0.2' style='width: 140px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Private Key:</span>";
  html += "<input type='password' id='wg-private-key' value='" + wgPrivateKey + "' placeholder='Base64 private key' style='width: 140px; padding-right: 8px;' autocomplete='off'>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Peer Public Key:</span>";
  html += "<input type='password' id='wg-public-key' value='" + wgPublicKey + "' placeholder='Base64 private key' style='width: 140px; padding-right: 8px;' autocomplete='off'>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Endpoint:</span>";
  html += "<input type='text' id='wg-endpoint' value='" + wgEndpoint + "' placeholder='vpn.example.com' style='width: 140px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Endpoint Port:</span>";
  html += "<input type='text' id='wg-endpoint-port' value='" + String(wgEndpointPort) + "' placeholder='51820' style='width: 140px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>DNS Server:</span>";
  html += "<input type='text' id='wg-dns' value='" + wgDns + "' placeholder='10.0.0.1' style='width: 140px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Allowed IPs:</span>";
  html += "<input type='text' id='wg-allowed-ips' value='" + wgAllowedIps + "' placeholder='0.0.0.0/0' style='width: 140px; padding-right: 8px;'>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>WireGuard requires NTP time sync to be enabled for the handshake</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' type='button' onclick='saveWgConfig()'>Save</button>";
  html += "<button class='btn btn-danger' type='button' onclick='resetWgConfig()'>Reset to Default</button>";
  html += "</div>";
  html += "</form>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  // JavaScript functions
  html += "<script>";

  // Styled modal helpers
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

  // Save WireGuard service (enable/disable)
  html += "function saveWgService() {";
  html += "  var enabled = document.getElementById('wg-enabled').checked ? '1' : '0';";
  html += "  showConfirm('Save WireGuard service setting and reboot?', function() {";
  html += "    fetch('/api/save-wg-service', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'enabled=' + enabled";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  // Reset WireGuard service to default
  html += "function resetWgService() {";
  html += "  showConfirm('Reset WireGuard service to default and reboot?', function() {";
  html += "    fetch('/api/reset-wg-service', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  // Save WireGuard tunnel configuration
  html += "function saveWgConfig() {";
  html += "  var localIp = document.getElementById('wg-local-ip').value;";
  html += "  var privateKey = document.getElementById('wg-private-key').value;";
  html += "  var publicKey = document.getElementById('wg-public-key').value;";
  html += "  var endpoint = document.getElementById('wg-endpoint').value;";
  html += "  var port = document.getElementById('wg-endpoint-port').value;";
  html += "  if (!localIp) { showAlert('Local IP is required'); return; }";
  html += "  if (!privateKey) { showAlert('Private key is required'); return; }";
  html += "  if (!publicKey) { showAlert('Peer public key is required'); return; }";
  html += "  if (!endpoint) { showAlert('Endpoint address is required'); return; }";
  html += "  if (port < 1 || port > 65535) { showAlert('Port must be 1-65535'); return; }";
  html += "  showConfirm('Save WireGuard configuration and reboot?<br><br>Endpoint: ' + endpoint + ':' + port, function() {";
  html += "    fetch('/api/save-wg-config', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'local_ip=' + encodeURIComponent(localIp) + '&private_key=' + encodeURIComponent(privateKey) + '&public_key=' + encodeURIComponent(publicKey) + '&endpoint=' + encodeURIComponent(endpoint) + '&port=' + port + '&dns=' + encodeURIComponent(document.getElementById('wg-dns').value) + '&allowed_ips=' + encodeURIComponent(document.getElementById('wg-allowed-ips').value)";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  // Reset WireGuard config to defaults
  html += "function resetWgConfig() {";
  html += "  showConfirm('Reset WireGuard configuration to defaults and reboot?', function() {";
  html += "    fetch('/api/reset-wg-config', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";

  html += "</script>";

  html += "</div>"; // Close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SYSTEM_WIREGUARD_H
