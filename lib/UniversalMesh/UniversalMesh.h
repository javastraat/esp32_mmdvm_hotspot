#ifndef UNIVERSAL_MESH_H
#define UNIVERSAL_MESH_H

#include <Arduino.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <espnow.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <esp_now.h>
  #include <esp_wifi.h>
#endif

// --- PACKET TYPES ---
#define MESH_TYPE_PING   0x12
#define MESH_TYPE_PONG   0x13
#define MESH_TYPE_ACK    0x14
#define MESH_TYPE_DATA   0x15

// --- UNIVERSAL MESH PACKET ---
struct __attribute__((packed)) MeshPacket {
  uint8_t  type;        
  uint8_t  ttl;         
  uint32_t msgId;       
  uint8_t  destMac[6];  
  uint8_t  srcMac[6];   
  uint8_t  appId;       
  uint8_t  payloadLen;  
  uint8_t  payload[64]; 
};

// Callback signature for the main sketch
typedef void (*MeshReceiveCallback)(MeshPacket* packet, uint8_t* senderMac);

class UniversalMesh {
  public:
    UniversalMesh();
    
    // Initialize the mesh on a specific Wi-Fi channel
    // Set isCoordinator=true so PONG replies carry payload[0]=0x01 (coordinator role flag)
    bool begin(uint8_t channel, bool isCoordinator = false);
    
    // Send a payload to a specific MAC (or broadcast)
    bool send(uint8_t destMac[6], uint8_t type, uint8_t appId, const uint8_t* payload, uint8_t len, uint8_t ttl = 4);
    
    // Register a function to handle incoming messages meant for this node
    void onReceive(MeshReceiveCallback callback);

    // Give the library a chance to process background tasks (WDT safe)
    void update(); 

  private:
    uint8_t _myMac[6];
    bool    _isCoordinator = false;
    uint8_t _broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    MeshReceiveCallback _userCallback;

    // Type-Aware Deduplication Engine
    struct SeenMessage { uint32_t msgId; uint8_t type; };
    static const int SEEN_SIZE = 30;
    SeenMessage _seen[SEEN_SIZE];
    uint8_t _sIdx = 0;
    bool isSeen(uint32_t id, uint8_t type);
    void markSeen(uint32_t id, uint8_t type);

    // Hardware Callback Wrappers (Static due to ESP-NOW requirements)
    static UniversalMesh* _instance;
    
    #if defined(ESP8266)
      static void espNowRecvWrapper(uint8_t *mac, uint8_t *data, uint8_t len);
    #elif defined(ESP32)
      static void espNowRecvWrapper(const esp_now_recv_info_t *info, const uint8_t *data, int len);
    #endif
    
    void handleReceive(uint8_t *mac, uint8_t *data, uint8_t len);
};

#endif