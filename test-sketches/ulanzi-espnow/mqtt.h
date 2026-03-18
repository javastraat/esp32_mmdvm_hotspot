#pragma once
// mqtt.h — MQTT + Home Assistant auto-discovery declarations.
// Config globals are DEFINED in mqtt.cpp; NVS load/save is in nvs_settings.cpp.
#include <Arduino.h>

// ── Config (persisted to NVS) ────────────────────────────────────────────────

extern bool     mqttEnabled;      // NVS: "mqtt_en"
extern char     mqttBroker[64];   // NVS: "mqtt_broker"
extern uint16_t mqttPort;         // NVS: "mqtt_port"    default 1883
extern char     mqttUser[32];     // NVS: "mqtt_user"
extern char     mqttPass[64];     // NVS: "mqtt_pass"
extern bool     mqttDiscovery;    // NVS: "mqtt_disc"    HA auto-discovery
extern char     mqttPrefix[32];   // NVS: "mqtt_prefix"  default "homeassistant"
extern char     mqttNodeId[32];   // NVS: "mqtt_node"    default "ulanzi"
extern char     mqttHaName[32];   // NVS: "mqtt_ha_name" Home Assistant device name

// ── Runtime API ───────────────────────────────────────────────────────────────
void        initMqttTask();        // start FreeRTOS task (call from setup after WiFi)
void        mqttNotifyPocsag();    // immediate POCSAG publish (call from processPocsagPacket)
void        mqttNotifyState();     // request immediate state publish (safe to call from any task)
void        mqttRequestReconnect();// drop + reconnect (call after config change)
bool        mqttIsConnected();
const char* mqttGetStatus();       // human-readable status string for web UI
