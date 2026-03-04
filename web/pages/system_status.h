/*
 * system_status.h - Status Page for ESP32 RTOS MMDVM Web Interface
 */

#ifndef WEB_SYSTEM_STATUS_H
#define WEB_SYSTEM_STATUS_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESP.h>
#include <SD.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

// WiFi / Network
extern bool softAPActive;
extern volatile bool ethConnected;
extern bool ethEnabled;
extern bool dnsFallbackEnabled;
extern String dnsFallbackIp;

// MMDVM / Modem
extern bool mmdvmReady;
extern String modemFirmwareVersion;
extern uint32_t dmrRxFreq;
extern uint32_t dmrTxFreq;
extern uint8_t dmrColorCode;
extern uint8_t dmrRfPower;

// Station
extern String userCallsign;
extern uint32_t userDmrId;
extern uint8_t userDmrSsid;
extern bool modeDmrEnabled;
extern bool modeDstarEnabled;
extern bool modeYsfEnabled;
extern bool modeP25Enabled;
extern bool modeNxdnEnabled;
extern bool modePocsagEnabled;

// MQTT
extern bool mqttEnabled;
extern bool mqttConnected;
extern String mqttBroker;
extern uint16_t mqttPort;
extern String mqttUser;
extern String mqttStatusTopic;

// WireGuard
extern bool wgEnabled;
extern bool wireguardConnected;
extern String wgLocalIp;
extern String wgEndpoint;
extern uint16_t wgEndpointPort;
extern String wgDns;
extern String wgAllowedIps;

// SD Card
extern bool sdcardEnabled;
extern bool sdCardMounted;
extern uint64_t sdCardSize;
extern uint64_t sdCardUsed;

// Firmware
extern String firmwareVersion;

String getSystemStatusPageHTML()
{
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>System Status - ESP32 RTOS MMDVM</title>";
  html += getSharedStyles();
  html += "<style>";
  html += ".status-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 15px; margin: 20px 0; }";
  html += ".metric { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #eee; }";
  html += ".metric:last-child { border-bottom: none; }";
  html += ".metric-label { font-weight: bold; color: #555; }";
  html += ".metric-value { color: #333; }";
  html += ".uptime { color: #007bff; font-weight: bold; }";
  html += ".refresh-info { color: var(--text-color); font-size: 0.9em; margin: 10px 0; padding: 8px; background: var(--info-bg); border-radius: 4px; text-align: center; }";
  html += ".controls { margin: 15px 0; padding: 10px; background: var(--card-bg); border-radius: 4px; text-align: center; }";
  html += ".btn { padding: 8px 16px; margin: 5px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer; }";
  html += ".btn:hover { background: #0056b3; }";
  html += ".mode-badges { display: flex; flex-wrap: wrap; gap: 5px; }";
  html += ".mode-badge { display: inline-block; padding: 3px 10px; background: #007bff; color: white; border-radius: 12px; font-size: 12px; font-weight: bold; }";
  html += "</style>";

  html += "<script>";
  html += "let autoRefresh = true;";
  html += "function updateStatus() {";
  html += "  if (!autoRefresh) return;";
  html += "  fetch('/api/status').then(r => r.json()).then(data => {";
  html += "    renderStatus(data);";
  html += "  }).catch(e => console.log('Failed to fetch status:', e));";
  html += "}";
  html += "function renderStatus(data) {";
  html += "  let html = '<div class=\"status-grid\">';";

  // Station Information Card
  html += "  html += '<div class=\"card\"><h3>Station Information</h3>';";
  html += "  var callsignClass = data.station.isDefaultCallsign ? 'disconnected' : 'connected';";
  html += "  var callsignText = data.station.isDefaultCallsign ? 'Callsign: N0CALL (Not Configured)' : 'Callsign: ' + data.station.callsign;";
  html += "  html += '<div class=\"status ' + callsignClass + '\">' + callsignText + '</div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">DMR ID:</span><span class=\"metric-value\">' + data.station.dmrId + '</span></div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">SSID:</span><span class=\"metric-value\">' + (data.station.ssid === 0 ? 'None' : data.station.ssid) + '</span></div>';";
  html += "  var modesHtml = '<div class=\"mode-badges\">';";
  html += "  if (data.station.modes && data.station.modes.length > 0) { data.station.modes.forEach(function(m) { modesHtml += '<span class=\"mode-badge\">' + m + '</span>'; }); }";
  html += "  else { modesHtml += 'None'; }";
  html += "  modesHtml += '</div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">Enabled Modes:</span><span class=\"metric-value\">' + modesHtml + '</span></div>';";
  html += "  html += '</div>';";

  // WiFi Status Card
  html += "  html += '<div class=\"card\"><h3>WiFi Status</h3>';";
  html += "  if (data.wifi.connected) {";
  html += "    html += '<div class=\"status connected\">Status: Connected</div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">SSID:</span><span class=\"metric-value\">' + data.wifi.ssid + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">IP Address:</span><span class=\"metric-value\">' + data.wifi.ip + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Gateway:</span><span class=\"metric-value\">' + (data.wifi.gateway || '-') + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Subnet Mask:</span><span class=\"metric-value\">' + (data.wifi.subnet || '-') + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">DNS Server:</span><span class=\"metric-value\">' + (data.wifi.dns1 || '-') + '</span></div>';";
  html += "    if (data.wifi.dns2) html += '<div class=\"metric\"><span class=\"metric-label\">DNS 2:</span><span class=\"metric-value\">' + data.wifi.dns2 + '</span></div>';";
  html += "    if (data.wifi.dnsBackup) html += '<div class=\"metric\"><span class=\"metric-label\">Backup DNS:</span><span class=\"metric-value\">' + data.wifi.dnsBackup + '</span></div>';";
  html += "    var rssi = data.wifi.rssi; var signalQuality = (rssi >= -50) ? 'Excellent' : (rssi >= -60) ? 'Good' : (rssi >= -70) ? 'Fair' : 'Weak';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Signal Strength:</span><span class=\"metric-value\">' + rssi + ' dBm (' + signalQuality + ')</span></div>';";
  html += "    if (data.wifi.channel) html += '<div class=\"metric\"><span class=\"metric-label\">Channel:</span><span class=\"metric-value\">' + data.wifi.channel + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">MAC Address:</span><span class=\"metric-value\">' + data.wifi.mac + '</span></div>';";
  html += "  } else if (data.wifi.apMode) {";
  html += "    html += '<div class=\"status warning\">Status: Access Point Mode</div>';";
  html += "    if (data.wifi.apSSID) html += '<div class=\"metric\"><span class=\"metric-label\">SSID:</span><span class=\"metric-value\">' + data.wifi.apSSID + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">AP IP:</span><span class=\"metric-value\">' + data.wifi.apIP + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Clients:</span><span class=\"metric-value\">' + data.wifi.clients + '</span></div>';";
  html += "    if (data.wifi.channel) html += '<div class=\"metric\"><span class=\"metric-label\">Channel:</span><span class=\"metric-value\">' + data.wifi.channel + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">MAC Address:</span><span class=\"metric-value\">' + data.wifi.mac + '</span></div>';";
  html += "  } else {";
  html += "    html += '<div class=\"status disconnected\">Status: Disconnected</div>';";
  html += "  }";
  html += "  html += '</div>';";

  // Ethernet Status Card (conditional)
  html += "  if (data.ethernet) {";
  html += "    html += '<div class=\"card\"><h3>Ethernet Status</h3>';";;
  html += "    if (data.ethernet.connected) {";
  html += "      html += '<div class=\"status connected\">Status: Connected</div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">IP Address:</span><span class=\"metric-value\">' + (data.ethernet.ip || '-') + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Gateway:</span><span class=\"metric-value\">' + (data.ethernet.gateway || '-') + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Subnet Mask:</span><span class=\"metric-value\">' + (data.ethernet.subnet || '-') + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">DNS Server:</span><span class=\"metric-value\">' + (data.ethernet.dns1 || '-') + '</span></div>';";
  html += "      if (data.ethernet.dns2) html += '<div class=\"metric\"><span class=\"metric-label\">DNS 2:</span><span class=\"metric-value\">' + data.ethernet.dns2 + '</span></div>';";
  html += "      if (data.ethernet.dnsBackup) html += '<div class=\"metric\"><span class=\"metric-label\">Backup DNS:</span><span class=\"metric-value\">' + data.ethernet.dnsBackup + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">MAC Address:</span><span class=\"metric-value\">' + (data.ethernet.mac || '-') + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Link Speed:</span><span class=\"metric-value\">' + (data.ethernet.linkSpeed !== undefined ? data.ethernet.linkSpeed + ' Mbps' : '-') + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Duplex:</span><span class=\"metric-value\">' + (data.ethernet.duplex || '-') + '</span></div>';";
  html += "    } else {";
  html += "      html += '<div class=\"status disconnected\">Status: Not Connected</div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">IP Address:</span><span class=\"metric-value\">' + (data.ethernet.ip || '-') + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Gateway:</span><span class=\"metric-value\">' + (data.ethernet.gateway || '-') + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Subnet Mask:</span><span class=\"metric-value\">' + (data.ethernet.subnet || '-') + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">DNS Server:</span><span class=\"metric-value\">' + (data.ethernet.dns1 || '-') + '</span></div>';";
  html += "      if (data.ethernet.dns2) html += '<div class=\"metric\"><span class=\"metric-label\">DNS 2:</span><span class=\"metric-value\">' + data.ethernet.dns2 + '</span></div>';";
  html += "      if (data.ethernet.dnsBackup) html += '<div class=\"metric\"><span class=\"metric-label\">Backup DNS:</span><span class=\"metric-value\">' + data.ethernet.dnsBackup + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">MAC Address:</span><span class=\"metric-value\">' + (data.ethernet.mac || '-') + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Link Speed:</span><span class=\"metric-value\">' + (data.ethernet.linkSpeed !== undefined ? data.ethernet.linkSpeed + ' Mbps' : '-') + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Duplex:</span><span class=\"metric-value\">' + (data.ethernet.duplex || '-') + '</span></div>';";
  html += "    }";
  html += "    html += '</div>';";
  html += "  }";

  // MMDVM Hardware Status Card
  html += "  html += '<div class=\"card\"><h3>MMDVM Hardware Status</h3>';";
  html += "  var mmdvmClass = data.mmdvm.ready ? 'connected' : 'disconnected';";
  html += "  html += '<div class=\"status ' + mmdvmClass + '\">Status: ' + (data.mmdvm.ready ? 'Ready' : 'Not Ready') + '</div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">RX Frequency:</span><span class=\"metric-value\">' + parseFloat(data.mmdvm.rxFreq).toFixed(4) + ' MHz</span></div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">TX Frequency:</span><span class=\"metric-value\">' + parseFloat(data.mmdvm.txFreq).toFixed(4) + ' MHz</span></div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">Color Code:</span><span class=\"metric-value\">' + data.mmdvm.colorCode + '</span></div>';";
  html += "  html += '<div class=\"metric\"><span class=\"metric-label\">Power Level:</span><span class=\"metric-value\">' + data.mmdvm.power + '</span></div>';";
  html += "  if (data.mmdvm.modemHw) {";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Modem:</span><span class=\"metric-value\">' + data.mmdvm.modemHw + '</span></div>';";
  html += "    if (data.mmdvm.modemVer) html += '<div class=\"metric\"><span class=\"metric-label\">Version:</span><span class=\"metric-value\">' + data.mmdvm.modemVer + '</span></div>';";
  html += "    if (data.mmdvm.modemDate) html += '<div class=\"metric\"><span class=\"metric-label\">Build Date:</span><span class=\"metric-value\">' + data.mmdvm.modemDate + '</span></div>';";
  html += "    if (data.mmdvm.modemCrystal) html += '<div class=\"metric\"><span class=\"metric-label\">Crystal:</span><span class=\"metric-value\">' + data.mmdvm.modemCrystal + '</span></div>';";
  html += "    if (data.mmdvm.modemChip) html += '<div class=\"metric\"><span class=\"metric-label\">Chip:</span><span class=\"metric-value\">' + data.mmdvm.modemChip + '</span></div>';";
  html += "    if (data.mmdvm.modemFwBy) html += '<div class=\"metric\"><span class=\"metric-label\">FW by:</span><span class=\"metric-value\">' + data.mmdvm.modemFwBy + '</span></div>';";
  html += "    if (data.mmdvm.modemGitId) html += '<div class=\"metric\"><span class=\"metric-label\">Git ID:</span><span class=\"metric-value\">' + data.mmdvm.modemGitId + '</span></div>';";
  html += "  } else if (data.mmdvm.modemFirmware) {";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Modem Firmware:</span><span class=\"metric-value\">' + data.mmdvm.modemFirmware + '</span></div>';";
  html += "  }";
  html += "  html += '</div>';";

  // WireGuard Status Card
  html += "  html += '<div class=\"card\"><h3>WireGuard Status</h3>';";
  html += "  if (data.wireguard && data.wireguard.enabled) {";
  html += "    if (data.wireguard.connected) {";
  html += "      html += '<div class=\"status connected\">Status: Connected</div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Local IP:</span><span class=\"metric-value\">' + data.wireguard.localIp + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Endpoint:</span><span class=\"metric-value\">' + data.wireguard.endpoint + ':' + data.wireguard.port + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">DNS:</span><span class=\"metric-value\">' + data.wireguard.dns + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Allowed IPs:</span><span class=\"metric-value\">' + data.wireguard.allowedIps + '</span></div>';";
  html += "    } else {";
  html += "      html += '<div class=\"status disconnected\">Status: Disconnected</div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Local IP:</span><span class=\"metric-value\">' + data.wireguard.localIp + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Endpoint:</span><span class=\"metric-value\">' + data.wireguard.endpoint + ':' + data.wireguard.port + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">DNS:</span><span class=\"metric-value\">' + data.wireguard.dns + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Allowed IPs:</span><span class=\"metric-value\">' + data.wireguard.allowedIps + '</span></div>';";
  html += "    }";
  html += "  } else {";
  html += "    html += '<div class=\"status disconnected\">Status: Disabled</div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Local IP:</span><span class=\"metric-value\">' + data.wireguard.localIp + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Endpoint:</span><span class=\"metric-value\">' + data.wireguard.endpoint + ':' + data.wireguard.port + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">DNS:</span><span class=\"metric-value\">' + data.wireguard.dns + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Allowed IPs:</span><span class=\"metric-value\">' + data.wireguard.allowedIps + '</span></div>';";
  html += "  }";
  html += "  html += '</div>';";

  // MQTT Status Card
  html += "  html += '<div class=\"card\"><h3>MQTT Status</h3>';";
  html += "  if (data.mqtt.enabled) {";
  html += "    var mqttClass = data.mqtt.connected ? 'connected' : 'disconnected';";
  html += "    html += '<div class=\"status ' + mqttClass + '\">Status: ' + (data.mqtt.connected ? 'Connected' : 'Disconnected') + '</div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Broker:</span><span class=\"metric-value\">' + (data.mqtt.broker || 'Not configured') + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Port:</span><span class=\"metric-value\">' + data.mqtt.port + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Username:</span><span class=\"metric-value\">' + (data.mqtt.username || 'None') + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Status Topic:</span><span class=\"metric-value\">' + (data.mqtt.statusTopic || 'Default') + '</span></div>';";
  html += "  } else {";
  html += "    html += '<div class=\"status disconnected\">Status: Disabled</div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Info:</span><span class=\"metric-value\">Enable in Settings/MQTT</span></div>';";
  html += "  }";
  html += "  html += '</div>';";

  // SD Card Status Card (conditional)
  html += "  if (data.sdCard) {";
  html += "    html += '<div class=\"card\"><h3>SD Card Status</h3>';";
  html += "    if (data.sdCard.mounted) {";
  html += "      html += '<div class=\"status connected\">Status: Available</div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Card Type:</span><span class=\"metric-value\">' + data.sdCard.type + '</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Card Size:</span><span class=\"metric-value\">' + data.sdCard.cardSizeMB + ' MB</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Total Space:</span><span class=\"metric-value\">' + data.sdCard.totalMB + ' MB</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Used Space:</span><span class=\"metric-value\">' + data.sdCard.usedMB + ' MB</span></div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Free Space:</span><span class=\"metric-value\">' + data.sdCard.freeMB + ' MB</span></div>';";
  html += "    } else {";
  html += "      html += '<div class=\"status disconnected\">Status: Not Available</div>';";
  html += "      html += '<div class=\"metric\"><span class=\"metric-label\">Info:</span><span class=\"metric-value\">No card inserted or mount failed</span></div>';";
  html += "    }";
  html += "    html += '</div>';";
  html += "  }";

  // // System Information Card
  // html += "  html += '<div class=\"card\"><h3>System Information</h3>';";
  // html += "  html += '<div class=\"metric\"><span class=\"metric-label\">Uptime:</span><span class=\"metric-value uptime\">' + data.system.uptime + '</span></div>';";
  // html += "  html += '<div class=\"metric\"><span class=\"metric-label\">Free Heap:</span><span class=\"metric-value\">' + data.system.freeHeap + ' KB</span></div>';";
  // html += "  html += '<div class=\"metric\"><span class=\"metric-label\">CPU Frequency:</span><span class=\"metric-value\">' + data.system.cpuFreq + ' MHz</span></div>';";
  // html += "  html += '<div class=\"metric\"><span class=\"metric-label\">Chip Model:</span><span class=\"metric-value\">' + data.system.chipModel + '</span></div>';";
  // html += "  if (data.system.firmware) {";
  // html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Firmware:</span><span class=\"metric-value\">' + data.system.firmware + '</span></div>';";
  // html += "  }";
  // html += "  html += '</div>';";

  html += "  html += '</div>';";
  html += "  document.getElementById('status-content').innerHTML = html;";
  html += "}";

  html += "function toggleAutoRefresh() {";
  html += "  autoRefresh = !autoRefresh;";
  html += "  document.getElementById('toggleBtn').textContent = autoRefresh ? 'Pause' : 'Resume';";
  html += "  document.getElementById('refresh-status').textContent = autoRefresh ? 'Auto-refreshing every 5 seconds...' : 'Auto-refresh paused';";
  html += "}";
  html += "setInterval(updateStatus, 5000);";
  html += "window.onload = updateStatus;";
  html += "</script>";

  html += "</head><body>";
  html += getNavigation("system-status");
  html += "<div class='container'>";
  html += "<h1>System Status</h1>";
  html += "<p>Live overview of system resources, active connections, and modem state.</p>";
  html += "<div id='status-content'>";
  // Initial content
  html += "<div class='status-grid'>";

  // Station Information Card
  html += "<div class='card'>";
  html += "<h3>Station Information</h3>";
  bool isDefaultCallsign = (userCallsign == "N0CALL");
  String callsignStatusClass = isDefaultCallsign ? "disconnected" : "connected";
  String callsignStatusText = isDefaultCallsign ? "Callsign: N0CALL (Not Configured)" : "Callsign: " + userCallsign;
  html += "<div class='status " + callsignStatusClass + "'>" + callsignStatusText + "</div>";
  html += "<div class='metric'><span class='metric-label'>DMR ID:</span><span class='metric-value'>" + String(userDmrId) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>SSID:</span><span class='metric-value'>" + (userDmrSsid == 0 ? String("None") : String(userDmrSsid)) + "</span></div>";
  String modesBadges = "<div class='mode-badges'>";
  bool hasMode = false;
  if (modeDmrEnabled) { modesBadges += "<span class='mode-badge'>DMR</span>"; hasMode = true; }
  if (modeDstarEnabled) { modesBadges += "<span class='mode-badge'>D-STAR</span>"; hasMode = true; }
  if (modeYsfEnabled) { modesBadges += "<span class='mode-badge'>YSF</span>"; hasMode = true; }
  if (modeP25Enabled) { modesBadges += "<span class='mode-badge'>P25</span>"; hasMode = true; }
  if (modeNxdnEnabled) { modesBadges += "<span class='mode-badge'>NXDN</span>"; hasMode = true; }
  if (modePocsagEnabled) { modesBadges += "<span class='mode-badge'>POCSAG</span>"; hasMode = true; }
  if (!hasMode) modesBadges += "None";
  modesBadges += "</div>";
  html += "<div class='metric'><span class='metric-label'>Enabled Modes:</span><span class='metric-value'>" + modesBadges + "</span></div>";
  html += "</div>";

  // WiFi Status Card
  html += "<div class='card'>";
  html += "<h3>WiFi Status</h3>";
  if (WiFi.status() == WL_CONNECTED)
  {
    html += "<div class='status connected'>Status: Connected</div>";
    html += "<div class='metric'><span class='metric-label'>SSID:</span><span class='metric-value'>" + WiFi.SSID() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>IP Address:</span><span class='metric-value'>" + WiFi.localIP().toString() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Gateway:</span><span class='metric-value'>" + WiFi.gatewayIP().toString() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Subnet Mask:</span><span class='metric-value'>" + WiFi.subnetMask().toString() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>DNS Server:</span><span class='metric-value'>" + WiFi.dnsIP(0).toString() + "</span></div>";
    int rssi = WiFi.RSSI();
    String signalQuality = (rssi >= -50) ? "Excellent" : (rssi >= -60) ? "Good"
                                                       : (rssi >= -70) ? "Fair"
                                                                       : "Weak";
    html += "<div class='metric'><span class='metric-label'>Signal Strength:</span><span class='metric-value'>" + String(rssi) + " dBm (" + signalQuality + ")</span></div>";
    html += "<div class='metric'><span class='metric-label'>Channel:</span><span class='metric-value'>" + String(WiFi.channel()) + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>MAC Address:</span><span class='metric-value'>" + WiFi.macAddress() + "</span></div>";
  }
  else if (softAPActive)
  {
    html += "<div class='status warning'>Status: Access Point Mode</div>";
    html += "<div class='metric'><span class='metric-label'>SSID:</span><span class='metric-value'>" + WiFi.softAPSSID() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>AP IP:</span><span class='metric-value'>" + WiFi.softAPIP().toString() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Clients:</span><span class='metric-value'>" + String(WiFi.softAPgetStationNum()) + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Channel:</span><span class='metric-value'>" + String(WiFi.channel()) + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>MAC Address:</span><span class='metric-value'>" + WiFi.softAPmacAddress() + "</span></div>";
  }
  else
  {
    html += "<div class='status disconnected'>Status: Disconnected</div>";
  }
  html += "</div>";

  // Ethernet Status Card
  if (ethEnabled)
  {
    html += "<div class='card'>";
    html += "<h3>Ethernet Status</h3>";
    if (ethConnected)
    {
      html += "<div class='status connected'>Status: Connected</div>";
      html += "<div class='metric'><span class='metric-label'>IP Address:</span><span class='metric-value'>" + ETH.localIP().toString() + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Gateway:</span><span class='metric-value'>" + ETH.gatewayIP().toString() + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Subnet Mask:</span><span class='metric-value'>" + ETH.subnetMask().toString() + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>DNS Server:</span><span class='metric-value'>" + ETH.dnsIP(0).toString() + "</span></div>";
      if (dnsFallbackEnabled && dnsFallbackIp.length() > 0)
        html += "<div class='metric'><span class='metric-label'>Backup DNS:</span><span class='metric-value'>" + dnsFallbackIp + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>MAC Address:</span><span class='metric-value'>" + ETH.macAddress() + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Link Speed:</span><span class='metric-value'>" + String(ETH.linkSpeed()) + " Mbps</span></div>";
      html += "<div class='metric'><span class='metric-label'>Duplex:</span><span class='metric-value'>" + String(ETH.fullDuplex() ? "Full" : "Half") + "</span></div>";
    }
    else
    {
      html += "<div class='status disconnected'>Status: Not Connected</div>";
      html += "<div class='metric'><span class='metric-label'>IP Address:</span><span class='metric-value'>" + ETH.localIP().toString() + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Gateway:</span><span class='metric-value'>" + ETH.gatewayIP().toString() + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Subnet Mask:</span><span class='metric-value'>" + ETH.subnetMask().toString() + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>DNS Server:</span><span class='metric-value'>" + ETH.dnsIP(0).toString() + "</span></div>";
      if (dnsFallbackEnabled && dnsFallbackIp.length() > 0)
        html += "<div class='metric'><span class='metric-label'>Backup DNS:</span><span class='metric-value'>" + dnsFallbackIp + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>MAC Address:</span><span class='metric-value'>" + ETH.macAddress() + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Link Speed:</span><span class='metric-value'>" + String(ETH.linkSpeed()) + " Mbps</span></div>";
      html += "<div class='metric'><span class='metric-label'>Duplex:</span><span class='metric-value'>" + String(ETH.fullDuplex() ? "Full" : "Half") + "</span></div>";
    }
    html += "</div>";
  }

  // MMDVM Hardware Status Card
  html += "<div class='card'>";
  html += "<h3>MMDVM Hardware Status</h3>";
  String mmdvmClass = mmdvmReady ? "connected" : "disconnected";
  html += "<div class='status " + mmdvmClass + "'>Status: " + (mmdvmReady ? "Ready" : "Not Ready") + "</div>";
  html += "<div class='metric'><span class='metric-label'>RX Frequency:</span><span class='metric-value'>" + String(dmrRxFreq / 1000000.0, 4) + " MHz</span></div>";
  html += "<div class='metric'><span class='metric-label'>TX Frequency:</span><span class='metric-value'>" + String(dmrTxFreq / 1000000.0, 4) + " MHz</span></div>";
  html += "<div class='metric'><span class='metric-label'>Color Code:</span><span class='metric-value'>" + String(dmrColorCode) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Power Level:</span><span class='metric-value'>" + String(dmrRfPower) + "</span></div>";
  // Parse modem firmware: "MMDVM_HS_Hat-v1.6.1 20260130 14.7456MHz ADF7021 FW by ... GitID #192cba3"
  if (modemFirmwareVersion.length() > 0)
  {
    String fw = modemFirmwareVersion;
    int dashV = fw.indexOf("-v");
    if (dashV > 0)
    {
      String hw = fw.substring(0, dashV);
      String rest = fw.substring(dashV + 1);
      // Split rest by spaces: v1.6.1 20260130 14.7456MHz ADF7021 FW by ...
      int s1 = rest.indexOf(' ');
      String ver = s1 > 0 ? rest.substring(0, s1) : rest;
      String r2 = s1 > 0 ? rest.substring(s1 + 1) : "";
      int s2 = r2.indexOf(' ');
      String buildDate = s2 > 0 ? r2.substring(0, s2) : r2;
      // Reformat YYYYMMDD to DD-MM-YYYY
      if (buildDate.length() == 8)
        buildDate = buildDate.substring(6, 8) + "-" + buildDate.substring(4, 6) + "-" + buildDate.substring(0, 4);
      String r3 = s2 > 0 ? r2.substring(s2 + 1) : "";
      int s3 = r3.indexOf(' ');
      String crystal = s3 > 0 ? r3.substring(0, s3) : r3;
      String r4 = s3 > 0 ? r3.substring(s3 + 1) : "";
      int s4 = r4.indexOf(' ');
      String chip = s4 > 0 ? r4.substring(0, s4) : r4;
      String gitId = "";
      int gi = fw.indexOf("GitID ");
      if (gi > 0)
        gitId = fw.substring(gi + 6);
      html += "<div class='metric'><span class='metric-label'>Modem:</span><span class='metric-value'>" + hw + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Version:</span><span class='metric-value'>" + ver + "</span></div>";
      if (buildDate.length() > 0)
        html += "<div class='metric'><span class='metric-label'>Build Date:</span><span class='metric-value'>" + buildDate + "</span></div>";
      if (crystal.length() > 0)
        html += "<div class='metric'><span class='metric-label'>Crystal:</span><span class='metric-value'>" + crystal + "</span></div>";
      if (chip.length() > 0)
        html += "<div class='metric'><span class='metric-label'>Chip:</span><span class='metric-value'>" + chip + "</span></div>";
      if (gitId.length() > 0)
        html += "<div class='metric'><span class='metric-label'>Git ID:</span><span class='metric-value'>" + gitId + "</span></div>";
    }
    else
    {
      html += "<div class='metric'><span class='metric-label'>Modem Firmware:</span><span class='metric-value'>" + fw + "</span></div>";
    }
  }
  html += "</div>";

  // WireGuard Status Card
  html += "<div class='card'>";
  html += "<h3>WireGuard Status</h3>";
  if (wgEnabled)
  {
    if (wireguardConnected)
    {
      html += "<div class='status connected'>Status: Connected</div>";
      html += "<div class='metric'><span class='metric-label'>Local IP:</span><span class='metric-value'>" + wgLocalIp + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Endpoint:</span><span class='metric-value'>" + wgEndpoint + ":" + String(wgEndpointPort) + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>DNS:</span><span class='metric-value'>" + wgDns + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Allowed IPs:</span><span class='metric-value'>" + wgAllowedIps + "</span></div>";
    }
    else
    {
      html += "<div class='status disconnected'>Status: Disconnected</div>";
      html += "<div class='metric'><span class='metric-label'>Local IP:</span><span class='metric-value'>" + wgLocalIp + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Endpoint:</span><span class='metric-value'>" + wgEndpoint + ":" + String(wgEndpointPort) + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>DNS:</span><span class='metric-value'>" + wgDns + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Allowed IPs:</span><span class='metric-value'>" + wgAllowedIps + "</span></div>";
    }
  }
  else
  {
    html += "<div class='status disconnected'>Status: Disabled</div>";
      html += "<div class='metric'><span class='metric-label'>Local IP:</span><span class='metric-value'>" + wgLocalIp + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Endpoint:</span><span class='metric-value'>" + wgEndpoint + ":" + String(wgEndpointPort) + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>DNS:</span><span class='metric-value'>" + wgDns + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Allowed IPs:</span><span class='metric-value'>" + wgAllowedIps + "</span></div>";
  }
  html += "</div>";

  // MQTT Status Card
  html += "<div class='card'>";
  html += "<h3>MQTT Status</h3>";
  if (mqttEnabled)
  {
    String mqttClass = mqttConnected ? "connected" : "disconnected";
    html += "<div class='status " + mqttClass + "'>Status: " + (mqttConnected ? "Connected" : "Disconnected") + "</div>";
    html += "<div class='metric'><span class='metric-label'>Broker:</span><span class='metric-value'>" + (mqttBroker.length() > 0 ? mqttBroker : String("Not configured")) + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Port:</span><span class='metric-value'>" + String(mqttPort) + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Username:</span><span class='metric-value'>" + (mqttUser.length() > 0 ? mqttUser : String("None")) + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Status Topic:</span><span class='metric-value'>" + (mqttStatusTopic.length() > 0 ? mqttStatusTopic : String("Default")) + "</span></div>";
  }
  else
  {
    html += "<div class='status disconnected'>Status: Disabled</div>";
    html += "<div class='metric'><span class='metric-label'>Info:</span><span class='metric-value'>Enable in Settings/MQTT</span></div>";
  }
  html += "</div>";

  // SD Card Status Card
  if (sdcardEnabled)
  {
    html += "<div class='card'>";
    html += "<h3>SD Card Status</h3>";
    if (sdCardMounted)
    {
      html += "<div class='status connected'>Status: Available</div>";
      uint8_t cardType = SD.cardType();
      String cardTypeName = cardType == CARD_MMC ? "MMC" : cardType == CARD_SD ? "SDSC"
                                                       : cardType == CARD_SDHC ? "SDHC"
                                                                               : "Unknown";
      float cardSizeMB = SD.cardSize() / (1024.0 * 1024.0);
      float totalMB = SD.totalBytes() / (1024.0 * 1024.0);
      float usedMB = SD.usedBytes() / (1024.0 * 1024.0);
      float freeMB = totalMB - usedMB;
      html += "<div class='metric'><span class='metric-label'>Card Type:</span><span class='metric-value'>" + cardTypeName + "</span></div>";
      html += "<div class='metric'><span class='metric-label'>Card Size:</span><span class='metric-value'>" + String(cardSizeMB, 2) + " MB</span></div>";
      html += "<div class='metric'><span class='metric-label'>Total Space:</span><span class='metric-value'>" + String(totalMB, 2) + " MB</span></div>";
      html += "<div class='metric'><span class='metric-label'>Used Space:</span><span class='metric-value'>" + String(usedMB, 2) + " MB</span></div>";
      html += "<div class='metric'><span class='metric-label'>Free Space:</span><span class='metric-value'>" + String(freeMB, 2) + " MB</span></div>";
    }
    else
    {
      html += "<div class='status disconnected'>Status: Not Available</div>";
      html += "<div class='metric'><span class='metric-label'>Info:</span><span class='metric-value'>No card inserted or mount failed</span></div>";
    }
    html += "</div>";
  }

  // // System Information Card
  // html += "<div class='card'>";
  // html += "<h3>System Information</h3>";

  // // Uptime calculation
  // unsigned long uptimeSeconds = millis() / 1000;
  // unsigned long days = uptimeSeconds / 86400;
  // unsigned long hours = (uptimeSeconds % 86400) / 3600;
  // unsigned long minutes = (uptimeSeconds % 3600) / 60;
  // unsigned long seconds = uptimeSeconds % 60;
  // String uptimeStr = "";
  // if (days > 0)
  //   uptimeStr += String(days) + "d ";
  // if (hours > 0 || days > 0)
  //   uptimeStr += String(hours) + "h ";
  // uptimeStr += String(minutes) + "m " + String(seconds) + "s";

  // html += "<div class='metric'><span class='metric-label'>Uptime:</span><span class='metric-value uptime'>" + uptimeStr + "</span></div>";
  // html += "<div class='metric'><span class='metric-label'>Free Heap:</span><span class='metric-value'>" + String(ESP.getFreeHeap() / 1024) + " KB</span></div>";
  // html += "<div class='metric'><span class='metric-label'>CPU Frequency:</span><span class='metric-value'>" + String(ESP.getCpuFreqMHz()) + " MHz</span></div>";
  // html += "<div class='metric'><span class='metric-label'>Chip Model:</span><span class='metric-value'>" + String(ESP.getChipModel()) + "</span></div>";
  // html += "<div class='metric'><span class='metric-label'>Firmware:</span><span class='metric-value'>" + firmwareVersion + "</span></div>";
  // html += "</div>";

  html += "</div>"; // Close status-grid
  html += "</div>"; // Close status-content

  html += "<div class='refresh-info' id='refresh-status'>Auto-refreshing every 5 seconds...</div>";
  html += "<div class='controls'>";
  html += "<button class='btn' onclick='updateStatus()'>Refresh Now</button>";
  html += "<button class='btn' id='toggleBtn' onclick='toggleAutoRefresh()'>Pause</button>";
  html += "</div>";

  html += "</div>"; // Close container
  html += getFooter();
  html += "</body></html>";

  return html;
}

// API endpoint for status data (JSON)
String getSystemStatusAPIJSON()
{
  String json = "{";

  // Station Information
  json += "\"station\":{";
  json += "\"callsign\":\"" + userCallsign + "\",";
  json += "\"isDefaultCallsign\":" + String((userCallsign == "N0CALL") ? "true" : "false") + ",";
  json += "\"dmrId\":" + String(userDmrId) + ",";
  json += "\"ssid\":" + String(userDmrSsid) + ",";
  json += "\"modes\":[";
  bool firstMode = true;
  if (modeDmrEnabled) { json += "\"DMR\""; firstMode = false; }
  if (modeDstarEnabled) { if (!firstMode) json += ","; json += "\"D-STAR\""; firstMode = false; }
  if (modeYsfEnabled) { if (!firstMode) json += ","; json += "\"YSF\""; firstMode = false; }
  if (modeP25Enabled) { if (!firstMode) json += ","; json += "\"P25\""; firstMode = false; }
  if (modeNxdnEnabled) { if (!firstMode) json += ","; json += "\"NXDN\""; firstMode = false; }
  if (modePocsagEnabled) { if (!firstMode) json += ","; json += "\"POCSAG\""; firstMode = false; }
  json += "]";
  json += "},";

  // WiFi Status
  json += "\"wifi\":{";
  json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"apMode\":" + String(softAPActive ? "true" : "false");
  if (WiFi.status() == WL_CONNECTED)
  {
    json += ",\"ssid\":\"" + WiFi.SSID() + "\"";
    json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
    json += ",\"rssi\":" + String(WiFi.RSSI());
    json += ",\"mac\":\"" + WiFi.macAddress() + "\"";
    json += ",\"gateway\":\"" + WiFi.gatewayIP().toString() + "\"";
    json += ",\"subnet\":\"" + WiFi.subnetMask().toString() + "\"";
    json += ",\"dns1\":\"" + WiFi.dnsIP(0).toString() + "\"";
    json += ",\"dns2\":\"" + WiFi.dnsIP(1).toString() + "\"";
    if (dnsFallbackEnabled && dnsFallbackIp.length() > 0) {
      json += ",\"dnsBackup\":\"" + dnsFallbackIp + "\"";
    }
  }
  else if (softAPActive)
  {
    json += ",\"apSSID\":\"" + WiFi.softAPSSID() + "\"";
    json += ",\"apIP\":\"" + WiFi.softAPIP().toString() + "\"";
    json += ",\"clients\":" + String(WiFi.softAPgetStationNum());
    json += ",\"channel\":" + String(WiFi.channel());
    json += ",\"mac\":\"" + WiFi.softAPmacAddress() + "\"";
  }
  json += "},";

  // Ethernet Status (conditional)
  if (ethEnabled)
  {
    json += "\"ethernet\":{";
    json += "\"connected\":" + String(ethConnected ? "true" : "false");
    if (ethConnected) {
      json += ",\"ip\":\"" + ETH.localIP().toString() + "\"";
      json += ",\"gateway\":\"" + ETH.gatewayIP().toString() + "\"";
      json += ",\"subnet\":\"" + ETH.subnetMask().toString() + "\"";
      json += ",\"dns1\":\"" + ETH.dnsIP(0).toString() + "\"";
      json += ",\"dns2\":\"" + ETH.dnsIP(1).toString() + "\"";
      if (dnsFallbackEnabled && dnsFallbackIp.length() > 0) {
        json += ",\"dnsBackup\":\"" + dnsFallbackIp + "\"";
      }
      json += ",\"mac\":\"" + ETH.macAddress() + "\"";
      json += ",\"linkSpeed\":" + String(ETH.linkSpeed());
      json += ",\"duplex\":\"" + String(ETH.fullDuplex() ? "Full" : "Half") + "\"";
    }
    json += "},";
  }

  // MMDVM Hardware Status
  json += "\"mmdvm\":{";
  json += "\"ready\":" + String(mmdvmReady ? "true" : "false") + ",";
  json += "\"rxFreq\":" + String(dmrRxFreq / 1000000.0, 4) + ",";
  json += "\"txFreq\":" + String(dmrTxFreq / 1000000.0, 4) + ",";
  json += "\"colorCode\":" + String(dmrColorCode) + ",";
  json += "\"power\":" + String(dmrRfPower);
  // Parse modem firmware into structured fields
  if (modemFirmwareVersion.length() > 0)
  {
    String fw = modemFirmwareVersion;
    int dashV = fw.indexOf("-v");
    if (dashV > 0)
    {
      json += ",\"modemHw\":\"" + fw.substring(0, dashV) + "\"";
      String rest = fw.substring(dashV + 1);
      int s1 = rest.indexOf(' ');
      if (s1 > 0)
      {
        json += ",\"modemVer\":\"" + rest.substring(0, s1) + "\"";
        rest = rest.substring(s1 + 1);
      }
      else
      {
        json += ",\"modemVer\":\"" + rest + "\"";
        rest = "";
      }
      int s2 = rest.indexOf(' ');
      if (s2 > 0)
      {
        String d = rest.substring(0, s2);
        if (d.length() == 8)
          d = d.substring(6, 8) + "-" + d.substring(4, 6) + "-" + d.substring(0, 4);
        json += ",\"modemDate\":\"" + d + "\"";
        rest = rest.substring(s2 + 1);
      }
      int s3 = rest.indexOf(' ');
      if (s3 > 0)
      {
        json += ",\"modemCrystal\":\"" + rest.substring(0, s3) + "\"";
        rest = rest.substring(s3 + 1);
      }
      int s4 = rest.indexOf(' ');
      if (s4 > 0)
      {
        json += ",\"modemChip\":\"" + rest.substring(0, s4) + "\"";
      }
      int fwBy = fw.indexOf("FW by ");
      int gi = fw.indexOf("GitID ");
      if (fwBy > 0)
      {
        int fwByEnd = (gi > fwBy) ? gi : fw.length();
        String fwByStr = fw.substring(fwBy + 6, fwByEnd);
        fwByStr.trim();
        json += ",\"modemFwBy\":\"" + fwByStr + "\"";
      }
      if (gi > 0)
      {
        json += ",\"modemGitId\":\"" + fw.substring(gi + 6) + "\"";
      }
    }
    else
    {
      json += ",\"modemFirmware\":\"" + fw + "\"";
    }
  }
  json += "},";

  // WireGuard Status
  json += "\"wireguard\":{";
  json += "\"enabled\":" + String(wgEnabled ? "true" : "false");
  json += ",\"connected\":" + String(wireguardConnected ? "true" : "false");
  json += ",\"localIp\":\"" + wgLocalIp + "\"";
  json += ",\"endpoint\":\"" + wgEndpoint + "\"";
  json += ",\"port\":" + String(wgEndpointPort);
  json += ",\"dns\":\"" + wgDns + "\"";
  json += ",\"allowedIps\":\"" + wgAllowedIps + "\"";
  json += "},";

  // MQTT Status
  json += "\"mqtt\":{";
  json += "\"enabled\":" + String(mqttEnabled ? "true" : "false") + ",";
  json += "\"connected\":" + String(mqttConnected ? "true" : "false") + ",";
  json += "\"broker\":\"" + mqttBroker + "\",";
  json += "\"port\":" + String(mqttPort) + ",";
  json += "\"username\":\"" + mqttUser + "\",";
  json += "\"statusTopic\":\"" + mqttStatusTopic + "\"";
  json += "},";

  // SD Card Status (conditional)
  if (sdcardEnabled)
  {
    json += "\"sdCard\":{";
    json += "\"mounted\":" + String(sdCardMounted ? "true" : "false");
    if (sdCardMounted)
    {
      uint8_t ct = SD.cardType();
      String ctn = ct == CARD_MMC ? "MMC" : ct == CARD_SD ? "SDSC"
                                        : ct == CARD_SDHC ? "SDHC"
                                                          : "Unknown";
      float cardSizeMB = SD.cardSize() / (1024.0 * 1024.0);
      float totalMB = SD.totalBytes() / (1024.0 * 1024.0);
      float usedMB = SD.usedBytes() / (1024.0 * 1024.0);
      float freeMB = totalMB - usedMB;
      json += ",\"type\":\"" + ctn + "\"";
      json += ",\"cardSizeMB\":" + String(cardSizeMB, 2);
      json += ",\"totalMB\":" + String(totalMB, 2);
      json += ",\"usedMB\":" + String(usedMB, 2);
      json += ",\"freeMB\":" + String(freeMB, 2);
    }
    json += "}";
  }

  // // System Information
  // json += "\"system\":{";

  // // Uptime calculation
  // unsigned long uptimeSeconds = millis() / 1000;
  // unsigned long days = uptimeSeconds / 86400;
  // unsigned long hours = (uptimeSeconds % 86400) / 3600;
  // unsigned long minutes = (uptimeSeconds % 3600) / 60;
  // unsigned long seconds = uptimeSeconds % 60;
  // String uptimeStr = "";
  // if (days > 0)
  //   uptimeStr += String(days) + "d ";
  // if (hours > 0 || days > 0)
  //   uptimeStr += String(hours) + "h ";
  // uptimeStr += String(minutes) + "m " + String(seconds) + "s";

  // json += "\"uptime\":\"" + uptimeStr + "\",";
  // json += "\"freeHeap\":" + String(ESP.getFreeHeap() / 1024) + ",";
  // json += "\"cpuFreq\":" + String(ESP.getCpuFreqMHz()) + ",";
  // json += "\"chipModel\":\"" + String(ESP.getChipModel()) + "\",";
  // json += "\"firmware\":\"" + firmwareVersion + "\"";
  // json += "}";

  json += "}";

  return json;
}

#endif // WEB_SYSTEM_STATUS_H
