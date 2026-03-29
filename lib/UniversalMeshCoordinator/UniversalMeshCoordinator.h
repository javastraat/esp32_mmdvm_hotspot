#ifndef UNIVERSAL_MESH_COORDINATOR_H
#define UNIVERSAL_MESH_COORDINATOR_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <mbedtls/aes.h>
#include "UniversalMesh.h"

// --- DEBUG MACROS ---
// Defaults to 1. To disable, add `-D UM_DEBUG_MODE=0` to build_flags in platformio.ini
#ifndef UM_DEBUG_MODE
#define UM_DEBUG_MODE 1 
#endif

#if UM_DEBUG_MODE
  #define UM_DEBUG_PRINT(x) Serial.print(x)
  #define UM_DEBUG_PRINTLN(x) Serial.println(x)
  #define UM_DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define UM_DEBUG_PRINT(x)
  #define UM_DEBUG_PRINTLN(x)
  #define UM_DEBUG_PRINTF(...)
#endif

class UniversalMeshCoordinator {
  public:
    UniversalMeshCoordinator();

    // Added mqttUser and mqttPass (defaulting to empty strings so it doesn't break open brokers)
    bool begin(uint8_t channel, const char* mqttBroker, uint16_t mqttPort, const char* clientId = "MeshBridge", const char* mqttUser = "", const char* mqttPass = "");

    void setNetworkKey(const char* key);

    void update();
  private:
    UniversalMesh _mesh;
    WiFiClient _wifiClient;
    PubSubClient _mqtt;

    uint8_t _publicKey[32]; 
    uint8_t _meshKey[16];
    bool _meshKeySet;

    String bytesToHex(const uint8_t* data, uint8_t len);
    bool decryptPayload(MeshPacket* packet, uint8_t* output, uint8_t* outputLen);
    void injectTimestamp(String& payload);

    String _brokerIp;
    uint16_t _brokerPort;
    String _clientId;
    String _mqttUser;
    String _mqttPass;
    
    unsigned long _lastMqttReconnect;

    void reconnectMQTT();

    static UniversalMeshCoordinator* _instance;
    static void meshCallbackWrapper(MeshPacket* packet, uint8_t* senderMac);
    void handleMeshMessage(MeshPacket* packet, uint8_t* senderMac);
};

#endif