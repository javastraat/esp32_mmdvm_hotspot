#include "UniversalMesh.h"
#include <esp_wifi.h>


UniversalMesh* UniversalMesh::_instance = nullptr;
uint8_t UniversalMesh::_coordinatorMac[6];

UniversalMesh::UniversalMesh() {
  _instance = this;
  _userCallback = nullptr;
  _role = MESH_NODE;
  _coordinatorFound = false;
  _lastDiscoveryPing = 0;
  _meshKeySet = false;
  memset(_coordinatorMac, 0, 6);
}

bool UniversalMesh::begin(uint8_t channel, MeshRole role) {
  _role = role;
  
  #if defined(ESP8266)
    wifi_set_channel(channel);
  #elif defined(ESP32)
    // ESP32 requires promiscuous mode to force a channel change while disconnected
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
  #endif
  
  #if defined(ESP32)
    esp_wifi_get_mac(WIFI_IF_STA, _myMac);
  #else
    WiFi.macAddress(_myMac);
  #endif

  if (esp_now_init() != 0) return false;

  #if defined(ESP8266)
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(espNowRecvWrapper);
    esp_now_add_peer(_broadcastMac, ESP_NOW_ROLE_COMBO, channel, NULL, 0);
  #elif defined(ESP32)
    esp_now_register_recv_cb(espNowRecvWrapper);
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, _broadcastMac, 6);
    peer.channel = channel;
    esp_now_add_peer(&peer);
  #endif
  UM_DEBUG_PRINTF("[MESH] ESP-NOW Initialized %02X:%02X:%02X:%02X:%02X:%02X\n", 
                  _myMac[0], _myMac[1], _myMac[2], _myMac[3], _myMac[4], _myMac[5]);
  return true;
}

void UniversalMesh::onReceive(MeshReceiveCallback callback) {
  _userCallback = callback;
}

void UniversalMesh::setNetworkKey(const char* key) {
  memset(_meshKey, 0, 16);
  strncpy((char*)_meshKey, key, 16);
  _meshKeySet = true;
}

bool UniversalMesh::send(uint8_t destMac[6], uint8_t type, uint8_t appId, const uint8_t* payload, uint8_t len, uint8_t ttl, bool encrypt) {
  MeshPacket p = {};
  p.type = type;
  p.ttl = ttl;
  
  #if defined(ESP8266)
    p.msgId = (uint32_t)os_random();
  #elif defined(ESP32)
    p.msgId = esp_random() % 1000000000; 
  #endif

  memcpy(p.destMac, destMac, 6);
  memcpy(p.srcMac, _myMac, 6);
  p.appId = appId;
  
  if (payload != nullptr && len > 0) {
    p.payloadLen = (len > 200) ? 200 : len;
    
    if (encrypt && _meshKeySet) {
      mbedtls_aes_context aes;
      mbedtls_aes_init(&aes);
      mbedtls_aes_setkey_enc(&aes, _meshKey, 128); // 128-bit AES Key
      
      uint8_t iv[16] = {0};
      memcpy(iv, &p.msgId, sizeof(p.msgId)); // Use unique MsgId as IV nonce
      
      size_t iv_offset = 0;
      mbedtls_aes_crypt_cfb128(&aes, MBEDTLS_AES_ENCRYPT, p.payloadLen, &iv_offset, iv, payload, p.payload);
      mbedtls_aes_free(&aes);
    } else {
      memcpy(p.payload, payload, p.payloadLen);
    }
  } else {
    p.payloadLen = 0;
  }

  return (esp_now_send(_broadcastMac, (uint8_t*)&p, sizeof(p)) == 0);
}

bool UniversalMesh::send(uint8_t destMac[6], uint8_t type, uint8_t appId, String payload, uint8_t ttl, bool encrypt) {
  return send(destMac, type, appId, (const uint8_t*)payload.c_str(), payload.length(), ttl, encrypt);
}

// --- LAZY SENDER LOGIC ---
bool UniversalMesh::sendToCoordinator(uint8_t appId, uint8_t* payload, uint8_t len) {
  if (!_coordinatorFound) return false; 
  return send(_coordinatorMac, MESH_TYPE_DATA, appId, payload, len, 4);
  
}

bool UniversalMesh::sendToCoordinator(uint8_t appId, String payload) {
  return sendToCoordinator(appId, (uint8_t*)payload.c_str(), payload.length());
}

bool UniversalMesh::sendSecureToCoordinator(uint8_t appId, String payload) {
  if (!_coordinatorFound) return false; 
  return send(_coordinatorMac, MESH_TYPE_SECURE_DATA, appId, payload, 4, true);
}

uint8_t UniversalMesh::findCoordinatorChannel(const char* nodeName) {
    uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    // Define the actual PING packet
    uint8_t pingData[] = {0x50, 0x49, 0x4E, 0x47}; 
    size_t pingLen = sizeof(pingData);

    for (uint8_t ch = 1; ch <= 13; ch++) {
        #if defined(ESP8266)
          wifi_set_channel(ch);
          if (esp_now_is_peer_exist(broadcastMac)) {
            esp_now_del_peer(broadcastMac);
          }
          esp_now_add_peer(broadcastMac, ESP_NOW_ROLE_COMBO, ch, NULL, 0);
        #elif defined(ESP32)
          esp_wifi_set_promiscuous(true);
          esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
          esp_wifi_set_promiscuous(false);
          
          esp_now_peer_info_t peerInfo = {};
          memcpy(peerInfo.peer_addr, broadcastMac, 6);
          peerInfo.channel = ch;
          peerInfo.encrypt = false;
          if (esp_now_is_peer_exist(broadcastMac)) {
              esp_now_mod_peer(&peerInfo);
          } else {
              esp_now_add_peer(&peerInfo);
          }
        #endif
        delay(10); 
        
        UM_DEBUG_PRINTF("[MESH] Pinging Channel %d...\n", ch);

        _pongReceived = false; 
        
        // Send the PING
        esp_now_send(broadcastMac, pingData, pingLen);

        // Listening Window
        unsigned long startWait = millis();
        while (millis() - startWait < 150) {
            if (_pongReceived) {
                UM_DEBUG_PRINTF("[MESH] PONG received on Channel %d!\n", ch);
                return ch; 
            }
            delay(5); // Let the CPU handle the incoming packet interrupt
        }
    }
    return 0;
}

// --- RTC MEMORY HELPERS ---
void UniversalMesh::setCoordinatorMac(uint8_t* mac) {
  memcpy(_coordinatorMac, mac, 6);
  _coordinatorFound = true;
}

void UniversalMesh::getCoordinatorMac(uint8_t* mac) {
  memcpy(mac, _coordinatorMac, 6);
}

bool UniversalMesh::isCoordinatorFound() {
  return _coordinatorFound;
}

// --- DEDUPLICATION ---
bool UniversalMesh::isSeen(uint32_t id, uint8_t type) {
  for(int i=0; i<SEEN_SIZE; i++) {
    if(_seen[i].msgId == id && _seen[i].type == type) return true;
  }
  return false;
}

void UniversalMesh::markSeen(uint32_t id, uint8_t type) {
  _seen[_sIdx] = {id, type};
  _sIdx = (_sIdx + 1) % SEEN_SIZE;
}

// --- RECEIVE LOGIC & AUTO-RELAY ---
#if defined(ESP8266)
void UniversalMesh::espNowRecvWrapper(uint8_t *mac, uint8_t *data, uint8_t len) {
  if (_instance) _instance->handleReceive(mac, data, len);
}
#elif defined(ESP32)
void UniversalMesh::espNowRecvWrapper(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (_instance) _instance->handleReceive((uint8_t*)info->src_addr, (uint8_t*)data, len);
}
#endif

void UniversalMesh::handleReceive(uint8_t *mac, uint8_t *data, uint8_t len) {
  // Handle raw PING/PONG for channel discovery
  if (len == 4 && memcmp(data, "PING", 4) == 0) {
    if (_role == MESH_COORDINATOR) {
      uint8_t pongData[] = {0x50, 0x4F, 0x4E, 0x47}; // "PONG"
      esp_now_send(_broadcastMac, pongData, 4);
    }
    return;
  }
  if (len == 4 && memcmp(data, "PONG", 4) == 0) {
    _pongReceived = true;
    memcpy(_coordinatorMac, mac, 6); 
    return;
  }

  if (len < sizeof(MeshPacket)) return;
  MeshPacket* p = (MeshPacket*)data;

  if (!isSeen(p->msgId, p->type)) {
    markSeen(p->msgId, p->type);
    
    // --- 1. PROTOCOL HANDSHAKES ---
    if (p->type == MESH_TYPE_PING) {
      // If we are the Coordinator, flag our PONG with appId 0xFF
      uint8_t replyAppId = (_role == MESH_COORDINATOR) ? 0xFF : 0x00;
      send(p->srcMac, MESH_TYPE_PONG, replyAppId, nullptr, 0, p->ttl);
      
      // If a Node sent a name in the PING, pass it up to the Coordinator's sketch!
      if (_role == MESH_COORDINATOR && p->payloadLen > 0 && _userCallback) {
        _userCallback(p, mac); 
      }
    }

    // Node silently catches the Coordinator's 0xFF PONG
    if (_role == MESH_NODE && p->type == MESH_TYPE_PONG && p->appId == 0xFF) {
      memcpy(_coordinatorMac, p->srcMac, 6);
      _coordinatorFound = true;
    }

    // --- 2. ROUTING LOGIC ---
    bool isForMe = (memcmp(p->destMac, _myMac, 6) == 0) || (memcmp(p->destMac, _broadcastMac, 6) == 0);
    bool terminalData = (p->type == MESH_TYPE_DATA && memcmp(p->destMac, _myMac, 6) == 0);

    // --- 3. APPLICATION CALLBACK ---
    if (isForMe && _userCallback && p->type != MESH_TYPE_PING) {
      _userCallback(p, mac);
    }

    // --- 4. AUTO-RELAY LOGIC ---
    if (p->ttl > 0 && !terminalData) {
      uint8_t relayBuffer[sizeof(MeshPacket)]; 
      memcpy(relayBuffer, data, len);
      ((MeshPacket*)relayBuffer)->ttl--;
      esp_now_send(_broadcastMac, relayBuffer, len);
    }
  }
}

void UniversalMesh::update() {
  // Background discovery if we lose the Coordinator
  if (_role == MESH_NODE && !_coordinatorFound) {
    if (millis() - _lastDiscoveryPing > 10000) { 
      _lastDiscoveryPing = millis();
      send(_broadcastMac, MESH_TYPE_PING, 0x00, nullptr, 0, 4);
    }
  }
}