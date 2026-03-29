#include "UniversalMesh.h"

UniversalMesh* UniversalMesh::_instance = nullptr;

UniversalMesh::UniversalMesh() {
  _instance = this;
  _userCallback = nullptr;
}

bool UniversalMesh::begin(uint8_t channel, bool isCoordinator) {
  _isCoordinator = isCoordinator;
  // WiFi.mode(WIFI_STA);
  // WiFi.disconnect();
  
  #if defined(ESP8266)
    wifi_set_channel(channel);
  #elif defined(ESP32)
    // esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE); // patched: STA mode, AP controls channel
  #endif
  
  #if defined(ESP32)
    esp_wifi_get_mac(WIFI_IF_STA, _myMac);
  #else
    WiFi.macAddress(_myMac);
  #endif

  if (esp_now_init() != 0) return false;

  // Register Receive Callback
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

  return true;
}

void UniversalMesh::onReceive(MeshReceiveCallback callback) {
  _userCallback = callback;
}

bool UniversalMesh::send(uint8_t destMac[6], uint8_t type, uint8_t appId, const uint8_t* payload, uint8_t len, uint8_t ttl) {
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
    p.payloadLen = (len > 64) ? 64 : len;
    memcpy(p.payload, payload, p.payloadLen);
  } else {
    p.payloadLen = 0;
  }

  return (esp_now_send(_broadcastMac, (uint8_t*)&p, sizeof(p)) == 0);
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
  if (len < sizeof(MeshPacket)) return;
  MeshPacket* p = (MeshPacket*)data;

  if (!isSeen(p->msgId, p->type)) {
    markSeen(p->msgId, p->type);
    
    // --- 1. PROTOCOL LAYER (Invisible to the application) ---
    if (p->type == MESH_TYPE_PING) {
      // payload[0]=0x01 means "I am the coordinator", 0x00 means regular node.
      // Sensor nodes use this to ignore PONGs from non-coordinators.
      uint8_t role = _isCoordinator ? 0x01 : 0x00;
      send(p->srcMac, MESH_TYPE_PONG, 0x00, &role, 1, p->ttl);
    }

    // --- 2. ROUTING LOGIC ---
    bool isForMe = (memcmp(p->destMac, _myMac, 6) == 0) || (memcmp(p->destMac, _broadcastMac, 6) == 0);
    bool terminalData = (p->type == MESH_TYPE_DATA && memcmp(p->destMac, _myMac, 6) == 0);

    // --- 3. APPLICATION LAYER CALLBACK ---
    if (isForMe && _userCallback && p->type != MESH_TYPE_PING) {
      _userCallback(p, mac);
    }

    // --- 4. AUTO-RELAY LOGIC ---
    if (p->ttl > 0 && !terminalData) {
      uint8_t relayBuffer[112]; 
      memcpy(relayBuffer, data, len);
      ((MeshPacket*)relayBuffer)->ttl--;
      
      #if defined(ESP8266)
        esp_now_send(_broadcastMac, relayBuffer, len);
      #elif defined(ESP32)
        esp_now_send(_broadcastMac, relayBuffer, len);
      #endif
    }
  }
}


void UniversalMesh::update() {
  // Placeholder for future buffer management to keep ESP8266 WDT happy
}