/*
 * mqtt_handlers.h - MQTT Configuration Handlers for ESP32 MMDVM Hotspot
 *
 * Contains handlers for:
 * - MQTT configuration save
 * - MQTT status monitoring
 */

#ifndef MQTT_HANDLERS_H
#define MQTT_HANDLERS_H

#include <Arduino.h>
#include <WebServer.h>
#include <PubSubClient.h>

// External variables
extern WebServer server;
extern PubSubClient mqttClient;
extern String dmr_callsign;
extern bool mqtt_enabled;
extern String mqtt_broker;
extern uint16_t mqtt_port;
extern String mqtt_username;
extern String mqtt_password;
extern String mqtt_client_id;
extern String mqtt_topic_prefix;
extern uint32_t mqtt_publish_interval;
extern bool mqttConnected;
extern unsigned long lastMqttPublish;
extern String lastMqttError;
extern unsigned long mqttConnectAttempts;

// External functions
extern bool checkAuthentication();
extern void logSerial(String message);
extern void saveConfig();

// ===== MQTT Configuration Handler =====

void handleSaveMqttConfig() {
  if (!checkAuthentication()) return;

  if (server.hasArg("enabled") && server.hasArg("broker") && server.hasArg("port") &&
      server.hasArg("client_id") && server.hasArg("prefix") && server.hasArg("interval")) {

    mqtt_enabled = (server.arg("enabled") == "1");
    mqtt_broker = server.arg("broker");
    mqtt_port = server.arg("port").toInt();
    mqtt_username = server.arg("username");
    mqtt_password = server.arg("password");
    mqtt_client_id = server.arg("client_id");
    mqtt_topic_prefix = server.arg("prefix");
    mqtt_publish_interval = server.arg("interval").toInt();

    // Validate values
    if (mqtt_port < 1 || mqtt_port > 65535) {
      server.send(400, "text/plain", "ERROR: Invalid port number (1-65535)");
      return;
    }

    if (mqtt_publish_interval < 5000 || mqtt_publish_interval > 300000) {
      server.send(400, "text/plain", "ERROR: Publish interval must be between 5000-300000 ms");
      return;
    }

    // Set defaults if fields are empty
    if (mqtt_client_id.length() == 0) {
      mqtt_client_id = dmr_callsign;
    }
    if (mqtt_topic_prefix.length() == 0) {
      mqtt_topic_prefix = "mmdvm/" + dmr_callsign;
    }

    saveConfig();

    // Disconnect existing MQTT connection to force reconnect with new settings
    if (mqttClient.connected()) {
      mqttClient.disconnect();
      mqttConnected = false;
    }

    String status = "SUCCESS: MQTT config saved - ";
    status += mqtt_enabled ? "Enabled" : "Disabled";
    if (mqtt_enabled) {
      status += " | Broker: " + mqtt_broker + ":" + String(mqtt_port);
    }
    server.send(200, "text/plain", status);
    logSerial(status);
  } else {
    server.send(400, "text/plain", "ERROR: Missing MQTT parameters");
  }
}

// ===== MQTT Monitor Handler =====

void handleMqttMonitor() {
  if (!checkAuthentication()) return;

  // Build JSON response
  String json = "{";
  json += "\"enabled\":" + String(mqtt_enabled ? "true" : "false") + ",";
  json += "\"connected\":" + String(mqttConnected ? "true" : "false") + ",";
  json += "\"broker\":\"" + mqtt_broker + "\",";
  json += "\"port\":" + String(mqtt_port) + ",";
  json += "\"client_id\":\"" + mqtt_client_id + "\",";
  json += "\"topic_prefix\":\"" + mqtt_topic_prefix + "\",";
  json += "\"publish_interval\":" + String(mqtt_publish_interval) + ",";
  json += "\"connect_attempts\":" + String(mqttConnectAttempts) + ",";
  json += "\"last_publish\":" + String(lastMqttPublish) + ",";
  json += "\"uptime\":" + String(millis() / 1000) + ",";

  if (lastMqttError.length() > 0) {
    json += "\"last_error\":\"" + lastMqttError + "\",";
  } else {
    json += "\"last_error\":null,";
  }

  // Add MQTT client state
  int mqttState = mqttClient.state();
  json += "\"mqtt_state\":" + String(mqttState) + ",";

  // State descriptions
  String stateDesc = "";
  switch (mqttState) {
    case -4: stateDesc = "MQTT_CONNECTION_TIMEOUT"; break;
    case -3: stateDesc = "MQTT_CONNECTION_LOST"; break;
    case -2: stateDesc = "MQTT_CONNECT_FAILED"; break;
    case -1: stateDesc = "MQTT_DISCONNECTED"; break;
    case 0: stateDesc = "MQTT_CONNECTED"; break;
    case 1: stateDesc = "MQTT_CONNECT_BAD_PROTOCOL"; break;
    case 2: stateDesc = "MQTT_CONNECT_BAD_CLIENT_ID"; break;
    case 3: stateDesc = "MQTT_CONNECT_UNAVAILABLE"; break;
    case 4: stateDesc = "MQTT_CONNECT_BAD_CREDENTIALS"; break;
    case 5: stateDesc = "MQTT_CONNECT_UNAUTHORIZED"; break;
    default: stateDesc = "UNKNOWN"; break;
  }
  json += "\"state_description\":\"" + stateDesc + "\",";

  // Add topics info
  json += "\"topics\":{";
  json += "\"system_info\":\"" + mqtt_topic_prefix + "/system/info\",";
  json += "\"modem_status\":\"" + mqtt_topic_prefix + "/modem/status\",";
  json += "\"network_status\":\"" + mqtt_topic_prefix + "/network/status\",";
  json += "\"slot1_activity\":\"" + mqtt_topic_prefix + "/slot1/activity\",";
  json += "\"slot2_activity\":\"" + mqtt_topic_prefix + "/slot2/activity\",";
  json += "\"availability\":\"" + mqtt_topic_prefix + "/availability\"";
  json += "}";

  json += "}";

  server.send(200, "application/json", json);
}

#endif // MQTT_HANDLERS_H
