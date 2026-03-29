#ifndef UNIVERSAL_MESH_H
#define UNIVERSAL_MESH_H

#include <Arduino.h>

#include <mbedtls/aes.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <espnow.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <esp_now.h>
  #include <esp_wifi.h>
#endif
#ifdef UM_DEBUG_MODE
  #define UM_DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
  #define UM_DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
  #define UM_DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define UM_DEBUG_PRINT(...)
  #define UM_DEBUG_PRINTLN(...)
  #define UM_DEBUG_PRINTF(...)
#endif

// --- PACKET TYPES ---
#define MESH_TYPE_PING          0x12
#define MESH_TYPE_PONG          0x13
#define MESH_TYPE_ACK           0x14
#define MESH_TYPE_DATA          0x15
#define MESH_TYPE_KEY_REQ       0x16  // Node asks for key, Coordinator replies with key
#define MESH_TYPE_SECURE_DATA   0x17  // Coordinator decrypts and forwards
#define MESH_TYPE_PARANOID_DATA 0x18  // Coordinator acts as a dumb pipe (End-to-End Encrypted)

// --- ROLES ---
enum MeshRole {
  MESH_NODE = 0,
  MESH_COORDINATOR = 1
};

// --- UNIVERSAL MESH PACKET ---
struct __attribute__((packed)) MeshPacket {
  uint8_t  type;        
  uint8_t  ttl;         
  uint32_t msgId;       
  uint8_t  destMac[6];  
  uint8_t  srcMac[6];   
  uint8_t  appId;       
  uint8_t  payloadLen;  
  uint8_t  payload[200]; 
};

// Callback signature for the main sketch
typedef void (*MeshReceiveCallback)(MeshPacket* packet, uint8_t* senderMac);

class UniversalMesh {
  public:
    UniversalMesh();
    
    // Initialize the mesh on a specific Wi-Fi channel with a specific role
    bool begin(uint8_t channel, MeshRole role = MESH_NODE);
    
    // Set a 16-character AES key for secure communications
    void setNetworkKey(const char* key);
    
    // Core send function
    bool send(uint8_t destMac[6], uint8_t type, uint8_t appId, const uint8_t* payload, uint8_t len, uint8_t ttl = 4, bool encrypt = false);
    bool send(uint8_t destMac[6], uint8_t type, uint8_t appId, String payload, uint8_t ttl = 4, bool encrypt = false);
    
    // The Lazy Sender: Shoots data directly to the known Coordinator
    bool sendToCoordinator(uint8_t appId, uint8_t* payload, uint8_t len);
    bool sendToCoordinator(uint8_t appId, String payload);
    bool sendSecureToCoordinator(uint8_t appId, String payload);

    // Channel Sweeper: Hunts for the Coordinator across all Wi-Fi channels
   uint8_t findCoordinatorChannel(const char* nodeName = nullptr);

    // RTC Memory Helpers
    void setCoordinatorMac(uint8_t* mac);
    void getCoordinatorMac(uint8_t* mac);
    bool isCoordinatorFound();
    
    // Register a function to handle incoming messages meant for this node
    void onReceive(MeshReceiveCallback callback);

    // Give the library a chance to process background tasks
    void update(); 

  private:
    uint8_t _myMac[6];
    uint8_t _broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    MeshReceiveCallback _userCallback;

    // State Management for Auto-Discovery
    MeshRole _role;
    static uint8_t _coordinatorMac[6];
    volatile bool _pongReceived;
    bool _coordinatorFound;
    unsigned long _lastDiscoveryPing;
    uint8_t _meshKey[16];
    bool _meshKeySet = false;

    static void espNowOnReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len);

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