/*
 * web_handlers_mqtt_settings.cpp - MQTT Settings Routes
 *
 * Extracted from web_handlers_settings.cpp.
 * All routes registered here use the global `server` object (extern WebServer server).
 *
 * Routes:
  *   /api/save-mqtt-service + reset
  *   /api/save-mqtt-broker + reset
  *   /api/save-mqtt-auth + reset
  *   /api/save-mqtt-topics + reset
  *   /api/save-mqtt-advanced + reset
 */

#include "system/web_handlers_mqtt_settings.h"
#include "system/system_webserver.h"
#include "system/system_logger.h"
#include "include/config.h"

extern bool mqttEnabled;
extern String mqttBroker;
extern uint16_t mqttPort;
extern String mqttUser;
extern String mqttPassword;
extern String mqttStatusTopic;
extern String mqttLogsTopic;
extern String mqttHardwareTopic;
extern String mqttLoggerTaskTopic;
extern String mqttOledTaskTopic;
extern String mqttLedTaskTopic;
extern String mqttWifiTaskTopic;
extern String mqttEthTaskTopic;
extern String mqttArduinoOtaTaskTopic;
extern String mqttNtpTaskTopic;
extern String mqttMqttClientTaskTopic;
extern String mqttWebServerTaskTopic;
extern String mqttSdCardTaskTopic;
extern String mqttSensorTaskTopic;
extern String mqttWgTaskTopic;
extern String mqttModemTaskTopic;
extern String mqttDmrTaskTopic;
extern String mqttDstarTaskTopic;
extern String mqttYsfTaskTopic;
extern String mqttP25TaskTopic;
extern String mqttNxdnTaskTopic;
extern String mqttPocsagTaskTopic;
extern String mqttSubscribeTopic;
extern uint16_t mqttSendHardwareInfo;
extern bool mqttHardwareInfoLog;
extern void saveSettings();

void registerMqttSettingsRoutes()
{
  // MQTT Service Settings
  server.on("/api/save-mqtt-service", HTTP_POST, []()
            {
    if (!server.hasArg("enabled")) {
      server.send(400, "text/plain", "ERROR: Missing parameter");
      return;
    }
    mqttEnabled = (server.arg("enabled") == "1");
    saveSettings();
    addLogMessage("[Settings] MQTT service: " + String(mqttEnabled ? "Enabled" : "Disabled"));
    server.send(200, "text/plain", "MQTT service saved: " + String(mqttEnabled ? "Enabled" : "Disabled")); });

  server.on("/api/reset-mqtt-service", HTTP_POST, []()
            {
    mqttEnabled = MQTT_ENABLED;
    saveSettings();
    addLogMessage("[Settings] MQTT service reset to default");
    server.send(200, "text/plain", "MQTT service reset to default"); });

  // MQTT Broker Settings
  server.on("/api/save-mqtt-broker", HTTP_POST, []()
            {
    if (!server.hasArg("broker") || !server.hasArg("port")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }
    String broker = server.arg("broker");
    uint16_t port = server.arg("port").toInt();
    if (broker.length() < 3) {
      server.send(400, "text/plain", "ERROR: Broker address too short");
      return;
    }
    if (port < 1 || port > 65535) {
      server.send(400, "text/plain", "ERROR: Invalid port");
      return;
    }
    mqttBroker = broker;
    mqttPort = port;
    saveSettings();
    addLogMessage("[Settings] MQTT broker: " + mqttBroker + ":" + String(mqttPort));
    server.send(200, "text/plain", "MQTT broker saved: " + mqttBroker + ":" + String(mqttPort)); });

  server.on("/api/reset-mqtt-broker", HTTP_POST, []()
            {
    mqttBroker = MQTT_BROKER;
    mqttPort = MQTT_PORT;
    saveSettings();
    addLogMessage("[Settings] MQTT broker reset to default");
    server.send(200, "text/plain", "MQTT broker reset to default"); });

  // MQTT Authentication
  server.on("/api/save-mqtt-auth", HTTP_POST, []()
            {
    if (!server.hasArg("user") || !server.hasArg("pass")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }
    mqttUser = server.arg("user");
    mqttPassword = server.arg("pass");
    saveSettings();
    addLogMessage("[Settings] MQTT authentication updated");
    server.send(200, "text/plain", "MQTT authentication saved"); });

  server.on("/api/reset-mqtt-auth", HTTP_POST, []()
            {
    mqttUser = MQTT_USER;
    mqttPassword = MQTT_PASSWORD;
    saveSettings();
    addLogMessage("[Settings] MQTT authentication reset to default");
    server.send(200, "text/plain", "MQTT authentication reset to default"); });

  // MQTT Topics
  server.on("/api/save-mqtt-topics", HTTP_POST, []()
            {
    if (!server.hasArg("status") || !server.hasArg("logs") || !server.hasArg("hw") || !server.hasArg("sub")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }
    mqttStatusTopic = server.arg("status");
    mqttLogsTopic = server.arg("logs");
    mqttHardwareTopic = server.arg("hw");
    mqttSubscribeTopic = server.arg("sub");
    saveSettings();
    addLogMessage("[Settings] MQTT topics updated");
    server.send(200, "text/plain", "MQTT topics saved"); });

  server.on("/api/reset-mqtt-topics", HTTP_POST, []()
            {
    mqttStatusTopic = MQTT_STATUS_TOPIC;
    mqttLogsTopic = MQTT_LOGS_TOPIC;
    mqttHardwareTopic = MQTT_HARDWARE_TOPIC;
    mqttSubscribeTopic = MQTT_SUBSCRIBE_TOPIC;
    saveSettings();
    addLogMessage("[Settings] MQTT topics reset to default");
    server.send(200, "text/plain", "MQTT topics reset to default"); });

  // MQTT Advanced Settings
  server.on("/api/save-mqtt-advanced", HTTP_POST, []()
            {
    if (!server.hasArg("interval") || !server.hasArg("hwlog")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }
    uint16_t interval = server.arg("interval").toInt();
    bool hwLog = (server.arg("hwlog") == "1");
    if (interval < 5 || interval > 3600) {
      server.send(400, "text/plain", "ERROR: Interval must be 5-3600 seconds");
      return;
    }
    mqttSendHardwareInfo = interval;
    mqttHardwareInfoLog = hwLog;
    saveSettings();
    addLogMessage("[Settings] MQTT advanced settings updated");
    server.send(200, "text/plain", "MQTT advanced settings saved"); });

  server.on("/api/reset-mqtt-advanced", HTTP_POST, []()
            {
    mqttSendHardwareInfo = MQTT_SEND_HARDWARE_INFO;
    mqttHardwareInfoLog = MQTT_HARDWARE_INFO_LOG;
    saveSettings();
    addLogMessage("[Settings] MQTT advanced settings reset to default");
    server.send(200, "text/plain", "MQTT advanced settings reset to default"); });

}
