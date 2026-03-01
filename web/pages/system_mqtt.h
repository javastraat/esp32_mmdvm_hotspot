/*
 * System MQTT Page
 * MQTT configuration and status
 */

#ifndef WEB_SYSTEM_MQTT_H
#define WEB_SYSTEM_MQTT_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

// External references to runtime MQTT settings
extern bool mqttEnabled;
extern String mqttBroker;
extern uint16_t mqttPort;
extern String mqttUser;
extern String mqttPassword;
extern String mqttStatusTopic;
extern String mqttLogsTopic;
extern String mqttHardwareTopic;
extern String mqttSubscribeTopic;
extern uint16_t mqttSendHardwareInfo;
extern bool mqttHardwareInfoLog;
String getSystemMqttPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>MQTT Configuration</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("system-mqtt");

  html += "<div class='container'>";
  html += "<h1>MQTT Configuration</h1>";
  html += "<p>Configure MQTT broker connection and topic settings</p>";

  html += "<div class='admin-grid'>";

  // Card 1: MQTT Enable/Disable
  html += "<div class='card'>";
  html += "<h3>MQTT Service</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enable MQTT:</span>";
  html += "<label class='switch'><input type='checkbox' id='mqtt-enabled'" + String(mqttEnabled ? " checked" : "") + "><span class='slider'></span></label>";
  html += "</div>";
  html += "<div class='metric'><span class='metric-label'>Current Status:</span><span class='metric-value'>" + String(mqttEnabled ? "Enabled" : "Disabled") + "</span></div>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveMqttService()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetMqttService()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 2: Broker Settings
  html += "<div class='card'>";
  html += "<h3>Broker Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Broker Address:</span>";
  html += "<input type='text' id='mqtt-broker' value='" + mqttBroker + "' placeholder='mqtt.example.com' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Port:</span>";
  html += "<input type='text' id='mqtt-port' value='" + String(mqttPort) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>Default: 1883 (MQTT), 8883 (MQTT over TLS)</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveMqttBroker()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetMqttBroker()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 3: Authentication
  html += "<div class='card'>";
  html += "<h3>Authentication</h3>";
  html += "<form onsubmit=\"return false;\">";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Username:</span>";
  html += "<input type='text' id='mqtt-user' value='" + mqttUser + "' placeholder='username' style='width: 120px; padding-right: 8px;' autocomplete='username'>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Password:</span>";
  html += "<input type='password' id='mqtt-pass' value='" + mqttPassword + "' placeholder='password' style='width: 120px; padding-right: 8px;' autocomplete='current-password'>";
  html += "</div>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' type='button' onclick='saveMqttAuth()'>Save</button>";
  html += "<button class='btn btn-danger' type='button' onclick='resetMqttAuth()'>Reset to Default</button>";
  html += "</div>";
  html += "</form>";
  html += "</div>";

  // Card 4: Topics Configuration
  html += "<div class='card'>";
  html += "<h3>Topics Configuration</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Status Topic:</span>";
  html += "<input type='text' id='mqtt-topic-status' value='" + mqttStatusTopic + "' placeholder='mmdvm/status' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Logs Topic:</span>";
  html += "<input type='text' id='mqtt-topic-logs' value='" + mqttLogsTopic + "' placeholder='mmdvm/logs' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Hardware Topic:</span>";
  html += "<input type='text' id='mqtt-topic-hw' value='" + mqttHardwareTopic + "' placeholder='mmdvm/hardware' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Subscribe Topic:</span>";
  html += "<input type='text' id='mqtt-topic-sub' value='" + mqttSubscribeTopic + "' placeholder='mmdvm/command' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveMqttTopics()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetMqttTopics()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 5: Advanced Settings
  html += "<div class='card'>";
  html += "<h3>Advanced Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>HW Info Interval (s):</span>";
  html += "<input type='text' id='mqtt-hw-interval' value='" + String(mqttSendHardwareInfo) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Log MQTT Info to Serial:</span>";
  html += "<label class='switch'><input type='checkbox' id='mqtt-hw-log'" + String(mqttHardwareInfoLog ? " checked" : "") + "><span class='slider'></span></label>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>When disabled, reduces serial log spam</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveMqttAdvanced()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetMqttAdvanced()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 6: Command Token
  html += "<div class='card'>";
  html += "<h3>Command Token</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Token required in MQTT commands. Leave empty to disable token check.</p>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Token:</span>";
  html += "<input type='text' id='mqtt-cmd-token' value='' placeholder='(none)' style='width:160px;padding-right:8px;font-family:monospace;' readonly>";
  html += "</div>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-primary' onclick='generateMqttToken()'>Generate</button>";
  html += "<button class='btn btn-success' onclick='saveMqttToken()'>Save</button>";
  html += "</div>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

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

  html += "function saveMqttService() {";
  html += "  var enabled = document.getElementById('mqtt-enabled').checked ? '1' : '0';";
  html += "  showConfirm('Save MQTT service setting and reboot?', function() {";
  html += "    fetch('/api/save-mqtt-service', {";
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
  html += "function resetMqttService() {";
  html += "  showConfirm('Reset MQTT service to default and reboot?', function() {";
  html += "    fetch('/api/reset-mqtt-service', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";
  html += "function saveMqttBroker() {";
  html += "  var broker = document.getElementById('mqtt-broker').value;";
  html += "  var port = document.getElementById('mqtt-port').value;";
  html += "  if (!broker || broker.length < 3) {";
  html += "    showAlert('Broker address must be at least 3 characters');";
  html += "    return;";
  html += "  }";
  html += "  if (port < 1 || port > 65535) {";
  html += "    showAlert('Port must be 1-65535');";
  html += "    return;";
  html += "  }";
  html += "  showConfirm('Save MQTT broker settings and reboot?<br><br>Broker: ' + broker + '<br>Port: ' + port, function() {";
  html += "    fetch('/api/save-mqtt-broker', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'broker=' + encodeURIComponent(broker) + '&port=' + port";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";
  html += "function resetMqttBroker() {";
  html += "  showConfirm('Reset MQTT broker to default and reboot?', function() {";
  html += "    fetch('/api/reset-mqtt-broker', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";
  html += "function saveMqttAuth() {";
  html += "  var user = document.getElementById('mqtt-user').value;";
  html += "  var pass = document.getElementById('mqtt-pass').value;";
  html += "  showConfirm('Save MQTT authentication and reboot?', function() {";
  html += "    fetch('/api/save-mqtt-auth', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'user=' + encodeURIComponent(user) + '&pass=' + encodeURIComponent(pass)";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";
  html += "function resetMqttAuth() {";
  html += "  showConfirm('Reset MQTT authentication to default and reboot?', function() {";
  html += "    fetch('/api/reset-mqtt-auth', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";
  html += "function saveMqttTopics() {";
  html += "  var status = document.getElementById('mqtt-topic-status').value;";
  html += "  var logs = document.getElementById('mqtt-topic-logs').value;";
  html += "  var hw = document.getElementById('mqtt-topic-hw').value;";
  html += "  var sub = document.getElementById('mqtt-topic-sub').value;";
  html += "  showConfirm('Save MQTT topics and reboot?', function() {";
  html += "    fetch('/api/save-mqtt-topics', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'status=' + encodeURIComponent(status) + '&logs=' + encodeURIComponent(logs) + '&hw=' + encodeURIComponent(hw) + '&sub=' + encodeURIComponent(sub)";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";
  html += "function resetMqttTopics() {";
  html += "  showConfirm('Reset MQTT topics to default and reboot?', function() {";
  html += "    fetch('/api/reset-mqtt-topics', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";
  html += "function saveMqttAdvanced() {";
  html += "  var interval = document.getElementById('mqtt-hw-interval').value;";
  html += "  var hwLog = document.getElementById('mqtt-hw-log').checked ? '1' : '0';";
  html += "  if (interval < 5 || interval > 3600) {";
  html += "    showAlert('Interval must be 5-3600 seconds');";
  html += "    return;";
  html += "  }";
  html += "  showConfirm('Save MQTT advanced settings and reboot?', function() {";
  html += "    fetch('/api/save-mqtt-advanced', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'interval=' + interval + '&hwlog=' + hwLog";
  html += "    }).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";
  html += "function resetMqttAdvanced() {";
  html += "  showConfirm('Reset MQTT advanced settings to default and reboot?', function() {";
  html += "    fetch('/api/reset-mqtt-advanced', {method: 'POST'}).then(r => r.text()).then(msg => {";
  html += "      showAlert(msg + '<br><br>The device will now reboot.');";
  html += "      fetch('/api/reboot', {method: 'POST'});";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';";
  html += "      setTimeout(function() { location.reload(); }, 10000);";
  html += "    });";
  html += "  });";
  html += "}";
  // Load current token on page load
  html += "fetch('/api/get-mqtt-token').then(r => r.text()).then(t => {";
  html += "  var el = document.getElementById('mqtt-cmd-token');";
  html += "  el.value = t;";
  html += "  el.readOnly = false;";
  html += "});";

  html += "function generateMqttToken() {";
  html += "  var arr = new Uint8Array(16);";
  html += "  crypto.getRandomValues(arr);";
  html += "  var token = Array.from(arr).map(b => b.toString(16).padStart(2,'0')).join('');";
  html += "  var el = document.getElementById('mqtt-cmd-token');";
  html += "  el.value = token;";
  html += "  el.readOnly = false;";
  html += "}";

  html += "function saveMqttToken() {";
  html += "  var token = document.getElementById('mqtt-cmd-token').value.trim();";
  html += "  showConfirm('Save command token?', function() {";
  html += "    fetch('/api/save-mqtt-token', {";
  html += "      method: 'POST',";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
  html += "      body: 'token=' + encodeURIComponent(token)";
  html += "    }).then(r => r.text()).then(msg => { showAlert(msg); });";
  html += "  });";
  html += "}";

  html += "</script>";

  html += "</div>"; // Close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SYSTEM_MQTT_H
