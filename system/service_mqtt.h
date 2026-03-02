/*
 * service_mqtt.h - MQTT Client for publishing system data
 */

#ifndef SERVICE_MQTT_H
#define SERVICE_MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>

// MQTT status
extern bool mqttConnected;
extern unsigned long lastMqttAttempt;

// Initialize MQTT task
void initMqttTask();

// Publish functions
bool publishMqtt(const char *topic, const char *payload);
bool publishMqtt(const char *topic, String payload);

// Status check
bool isMqttConnected();

#endif
