#include "UniversalMeshCoordinator.h"
#include <time.h>

UniversalMeshCoordinator* UniversalMeshCoordinator::_instance = nullptr;

UniversalMeshCoordinator::UniversalMeshCoordinator() : _mqtt(_wifiClient) {
    _instance = this;
    _lastMqttReconnect = 0;
    _meshKeySet = false;
    
    // Fill the public key with dummy data for now (0xAA)
    memset(_publicKey, 0xAA, 32); 
}

void UniversalMeshCoordinator::setNetworkKey(const char* key) {
    memset(_meshKey, 0, 16);
    strncpy((char*)_meshKey, key, 16);
    _meshKeySet = true;
}

bool UniversalMeshCoordinator::begin(uint8_t channel, const char* mqttBroker, uint16_t mqttPort, const char* clientId, const char* mqttUser, const char* mqttPass) {
    // Safely copy the strings into the class memory
    _brokerIp = mqttBroker;
    _brokerPort = mqttPort;
    _clientId = clientId;
    _mqttUser = mqttUser;
    _mqttPass = mqttPass;

    _mqtt.setServer(_brokerIp.c_str(), _brokerPort);
    _mqtt.setBufferSize(256); 

    // Start background NTP sync (UTC time)
    configTime(0, 0, "pool.ntp.org");

    if (_mesh.begin(channel, MESH_COORDINATOR)) {
        _mesh.onReceive(meshCallbackWrapper);
        UM_DEBUG_PRINTLN("[BRIDGE] Mesh Radio Online. Role: Coordinator.");
        return true;
    }
    UM_DEBUG_PRINTLN("[BRIDGE] FATAL: ESP-NOW Init Failed!");
    return false;
}

void UniversalMeshCoordinator::update() {
    _mesh.update();

    if (WiFi.status() == WL_CONNECTED) {
        if (!_mqtt.connected()) {
            if (millis() - _lastMqttReconnect > 5000) {
                _lastMqttReconnect = millis();
                reconnectMQTT();
            }
        } else {
            _mqtt.loop();
        }
    }
}

void UniversalMeshCoordinator::reconnectMQTT() {
    UM_DEBUG_PRINT("[BRIDGE] Connecting to MQTT Broker... ");
    
    bool connected = false;
    
    // Check if we were given a username. If so, use the authenticated connect()
    if (_mqttUser.length() > 0) {
        connected = _mqtt.connect(_clientId.c_str(), _mqttUser.c_str(), _mqttPass.c_str());
    } else {
        connected = _mqtt.connect(_clientId.c_str());
    }

    if (connected) {
        UM_DEBUG_PRINTLN("Connected!");
    } else {
        UM_DEBUG_PRINTF("Failed, rc=%d. Retrying in 5s.\n", _mqtt.state());
    }
}

void UniversalMeshCoordinator::meshCallbackWrapper(MeshPacket* packet, uint8_t* senderMac) {
    if (_instance) {
        _instance->handleMeshMessage(packet, senderMac);
    }
}

void UniversalMeshCoordinator::handleMeshMessage(MeshPacket* packet, uint8_t* senderMac) {
    char macStr[13];
    sprintf(macStr, "%02X%02X%02X%02X%02X%02X", 
            packet->srcMac[0], packet->srcMac[1], packet->srcMac[2],
            packet->srcMac[3], packet->srcMac[4], packet->srcMac[5]);

    // ---------------------------------------------------------
    // 1. STANDARD DATA (Unencrypted)
    // ---------------------------------------------------------
    if (packet->type == MESH_TYPE_DATA) {
        String topic = "mesh/telemetry/" + String(macStr) + "/" + String(packet->appId);
        
        // Safely extract the payload and ensure null-termination
        char safePayload[201];
        uint8_t safeLen = packet->payloadLen > 200 ? 200 : packet->payloadLen;
        memcpy(safePayload, packet->payload, safeLen);
        safePayload[safeLen] = '\0'; // Explicit null termination
        
        String textPayload(safePayload);
        injectTimestamp(textPayload);

        if (_mqtt.connected()) _mqtt.publish(topic.c_str(), textPayload.c_str());
        UM_DEBUG_PRINTF("[DATA] Received from %s: %s\n", macStr, textPayload.c_str());
    } 
    
    // ---------------------------------------------------------
    // 2. THE HANDSHAKE: PUBLIC KEY REQUEST
    // ---------------------------------------------------------
    else if (packet->type == MESH_TYPE_KEY_REQ) {
        UM_DEBUG_PRINTF("[SECURITY] Node %s requested Public Key. Sending...\n", macStr);
        
        // We reply using the same MESH_TYPE_KEY_REQ, but with our 32-byte public key as the payload
        // _publicKey would be generated in the Coordinator's setup()
        _mesh.send(packet->srcMac, MESH_TYPE_KEY_REQ, 0x00, _publicKey, 32, 4);
    }
    
    // ---------------------------------------------------------
    // 3. SECURE DATA (Coordinator Decrypts)
    // ---------------------------------------------------------
    else if (packet->type == MESH_TYPE_SECURE_DATA) {
        uint8_t decryptedBuffer[200];
        uint8_t decryptedLen = 0;
        
        // Attempt to decrypt using the Coordinator's Private Key
        if (decryptPayload(packet, decryptedBuffer, &decryptedLen)) {
            String topic = "mesh/secure/" + String(macStr) + "/" + String(packet->appId);
            
            // Safely extract the decrypted payload and ensure null-termination
            char safePayload[201];
            uint8_t safeLen = decryptedLen > 200 ? 200 : decryptedLen;
            memcpy(safePayload, decryptedBuffer, safeLen);
            safePayload[safeLen] = '\0'; // Explicit null termination
            
            String textPayload(safePayload);
            injectTimestamp(textPayload);

            if (_mqtt.connected()) _mqtt.publish(topic.c_str(), textPayload.c_str());
            UM_DEBUG_PRINTF("[SECURITY] Decrypted payload from %s: %s\n", macStr, textPayload.c_str());
        } else {
            UM_DEBUG_PRINTLN("[SECURITY] ERROR: Failed to decrypt payload!");
        }
    }
    
    // ---------------------------------------------------------
    // 4. PARANOID DATA (Coordinator is a dumb pipe)
    // ---------------------------------------------------------
    else if (packet->type == MESH_TYPE_PARANOID_DATA) {
        // We DO NOT touch the payload. We just convert the raw encrypted bytes to Hex
        // and send it to a special MQTT topic. Your Home Assistant or backend server 
        // will hold the private key and do the decryption there.
        String topic = "mesh/paranoid/" + String(macStr) + "/" + String(packet->appId);
        String hexPayload = bytesToHex(packet->payload, packet->payloadLen);
        
        if (_mqtt.connected()) _mqtt.publish(topic.c_str(), hexPayload.c_str());
        UM_DEBUG_PRINTF("[SECURITY] Forwarded PARANOID payload from %s\n", macStr);
    }
    
    else if (packet->type == MESH_TYPE_PING && packet->payloadLen > 0) {
        // Safely copy the node name and ensure it's null-terminated
        char nodeName[65];
        uint8_t nameLen = (packet->payloadLen < 64) ? packet->payloadLen : 64;
        memcpy(nodeName, packet->payload, nameLen);
        nodeName[nameLen] = '\0';
        UM_DEBUG_PRINTF("[DISCOVERY] New Node Joined: %s (%s)\n", macStr, nodeName);
    }
}

// Helper to convert binary payload to a clean Hex string
String UniversalMeshCoordinator::bytesToHex(const uint8_t* data, uint8_t length) {
  String hexString = "";
  hexString.reserve(length * 2); // Optimize memory allocation
  
  for (size_t i = 0; i < length; i++) {
    if (data[i] < 0x10) {
      hexString += "0"; // Add leading zero for single-digit hex values
    }
    hexString += String(data[i], HEX);
  }
  
  hexString.toUpperCase();
  return hexString;
}

// AES-128 CFB Decryption
bool UniversalMeshCoordinator::decryptPayload(MeshPacket* packet, uint8_t* output, uint8_t* outputLen) {
    if (!_meshKeySet) return false;

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    
    // CFB128 decryption paradoxically uses the ENCRYPTION key schedule
    mbedtls_aes_setkey_enc(&aes, _meshKey, 128); 
    
    uint8_t iv[16] = {0};
    memcpy(iv, &packet->msgId, sizeof(packet->msgId)); // Reconstruct IV from MsgId
    
    size_t iv_offset = 0;
    mbedtls_aes_crypt_cfb128(&aes, MBEDTLS_AES_DECRYPT, packet->payloadLen, &iv_offset, iv, packet->payload, output);
    
    *outputLen = packet->payloadLen;
    mbedtls_aes_free(&aes);
    return true; 
}

// Smart middleware helper: Injects a UNIX timestamp without heavy JSON deserialization
void UniversalMeshCoordinator::injectTimestamp(String& payload) {
    time_t now;
    time(&now);
    
    // If time > 1600000000 (Sept 2020), we know NTP has successfully synced
    if (now > 1600000000) { 
        if (payload.startsWith("{") && payload.endsWith("}")) {
            payload.remove(payload.length() - 1); // Remove trailing '}'
            payload += ",\"ts\":";
            payload += String((unsigned long)now);
            payload += "}";
        }
    }
}