/*
 * admin_network.h - Admin Network Page
 * MQTT configuration
 */

#ifndef WEB_PAGES_ADMIN_NETWORK_H
#define WEB_PAGES_ADMIN_NETWORK_H

#include <Arduino.h>
#include "../common/css.h"
#include "../common/navigation.h"
#include "../common/utils.h"

// Forward declarations of handler functions
void handleSaveMqttConfig();

// Forward declarations
extern bool mqtt_enabled;
extern String mqtt_broker;
extern uint16_t mqtt_port;
extern String mqtt_username;
extern String mqtt_password;
extern String mqtt_client_id;
extern String mqtt_topic_prefix;
extern uint32_t mqtt_publish_interval;
extern String device_hostname;
extern String dmr_callsign;

void handleAdminNetwork() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Network Settings - " + dmr_callsign + "</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".admin-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 20px 0; }";
  html += "@media (max-width: 768px) { .admin-grid { grid-template-columns: 1fr; } }";
  html += ".btn { display: inline-block; padding: 12px 24px; margin: 10px 5px; border: none; border-radius: 6px; cursor: pointer; font-size: 14px; font-weight: bold; }";
  html += ".btn-success { background: #28a745; color: white; }";
  html += ".btn-success:hover { background: #218838; }";
  html += "</style></head><body>";
  html += getNavigation("admin");
  html += "<div class='container'>";
  html += "<h1>Network Settings</h1>";

  // Start admin grid container
  html += "<div class='admin-grid'>";

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

  html += "</div>"; // Close admin-grid

  // JavaScript functions
  html += "<script>";
  html += "function togglePasswordField(fieldId) {";
  html += "  var field = document.getElementById(fieldId);";
  html += "  field.type = field.type === 'password' ? 'text' : 'password';";
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
  html += "</script>";

  html += "</div>";  // Close container
  html += getFooter();
  html += "</body></html>";
  server.send(200, "text/html; charset=UTF-8", html);
}

#endif // WEB_PAGES_ADMIN_NETWORK_H
