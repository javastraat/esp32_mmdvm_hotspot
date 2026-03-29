/*
 * system_espnow.cpp - ESP-NOW DMR/POCSAG relay via UniversalMesh
 *
 * Uses the UniversalMesh library (lib/UniversalMesh) for coordinator
 * discovery, deduplication, auto-relay, and heartbeat.
 *
 * Roles (runtime):
 *   Coordinator  (espnowSenderEnabled = true)
 *     Broadcasts DMR/POCSAG to all mesh nodes.
 *     Library auto-responds to PING with PONG(role=0x01).
 *     Tracks node announcements for the status endpoint.
 *     Sends periodic announce broadcast so nodes can refresh last-seen.
 *
 *   Node  (dmrServerEspNow || pocsagServerEspNow)
 *     Broadcasts PING, waits for PONG(role=0x01) to find coordinator.
 *     Enqueues received DMR/POCSAG to espnowDmrNetQueue / espnowPocsagQueue.
 *     Sends heartbeat + announce to coordinator every 60 s.
 *
 * Payload layout (all <= 64 bytes — MeshPacket payload limit):
 *   MESH_APP_DMR       (0x10): byte[0]=len, byte[1..len]=raw DMRD data (max 60)
 *   MESH_APP_POCSAG    (0x11): byte[0..3]=ric(LE), byte[4]=functional,
 *                               byte[5..63]=message null-terminated (max 58 chars)
 *   MESH_APP_HEARTBEAT (0x05): byte[0]=0x01
 *   MESH_APP_ANNOUNCE  (0x06): node name string
 *
 * POCSAG messages longer than 58 chars are truncated — hard limit of the
 * 64-byte MeshPacket payload field.
 */

#include "system/system_espnow.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_wifi.h>
#include "lib/UniversalMesh/UniversalMesh.h"
#include "system/system_logger.h"
#include "system/system_eth.h"
#include "system/system_wifi.h"

extern String   userCallsign;
extern uint8_t  userDmrSsid;
extern bool     dmrServerEspNow;
extern bool     pocsagServerEspNow;

// ── UDP Discovery responder ─────────────────────────────────────────────────
// Listens on UDP port 3491 for "ESPNOW_DISCOVER" broadcast pings (sent by
// external transmitter sketches) and replies with MAC + callsign so those
// devices can find this hotspot automatically.

#define ESPNOW_DISCOVERY_PORT 3491

static void espnowDiscoveryTask(void* param) {
  while (WiFi.status() != WL_CONNECTED && !ethConnected && !softAPActive) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  WiFiUDP udp;
  if (!udp.begin(ESPNOW_DISCOVERY_PORT)) {
    addLogMessage("[ESP-NOW] Discovery: failed to bind UDP port " + String(ESPNOW_DISCOVERY_PORT));
    vTaskDelete(nullptr);
    return;
  }
  addLogMessage("[ESP-NOW] Discovery responder on UDP port " + String(ESPNOW_DISCOVERY_PORT));
  for (;;) {
    int len = udp.parsePacket();
    if (len > 0) {
      char buf[64] = {};
      udp.read(buf, min(len, 63));
      if (strstr(buf, "ESPNOW_DISCOVER")) {
        String mac  = WiFi.macAddress();
        String name = userCallsign;
        if (userDmrSsid > 0) name += "-" + String(userDmrSsid);
        String resp = "{\"mac\":\"" + mac + "\""
                    + ",\"name\":\"" + name + "\""
                    + ",\"dmr_relay\":"    + (dmrServerEspNow    ? "true" : "false")
                    + ",\"pocsag_relay\":" + (pocsagServerEspNow ? "true" : "false")
                    + "}";
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.write((const uint8_t*)resp.c_str(), resp.length());
        udp.endPacket();
        addLogMessage("[ESP-NOW] Discovery: replied to " + udp.remoteIP().toString()
                    + " (" + name + ")");
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void initEspNowDiscovery() {
  xTaskCreatePinnedToCore(espnowDiscoveryTask, "espnow_disc", 3072, nullptr, 1, nullptr, 0);
}

// ============================================================================
#if ESPNOW_SENDER
// ============================================================================

// AppId assignments (per UniversalMesh protocol reference):
//   0x01  Text      POCSAG: human-readable  "P:<ric>,<func>,<message>"
//   0x02  Raw Hex   DMR:    raw DMRD bytes  (payloadLen = byte count)
//   0x05  Heartbeat node → coordinator alive ping
//   0x06  Announce  node broadcasts its name
#define MESH_APP_TEXT       0x01
#define MESH_APP_RAW        0x02
#define MESH_APP_HEARTBEAT  0x05
#define MESH_APP_ANNOUNCE   0x06

// POCSAG text format "P:<ric>,<func>,<msg>" in 64 bytes:
//   "P:" + 10(ric) + "," + 1(func) + "," + msg + null  →  max 48 chars of message
#define MESH_POCSAG_MSG_MAX 48

#define ESPNOW_MESH_CHANNEL  0      // 0 = follow current WiFi channel
#define HEARTBEAT_INTERVAL   60000  // ms: node → coordinator heartbeat
#define ANNOUNCE_INTERVAL    120000 // ms: coordinator broadcast announce

#define ESPNOW_MAX_PEERS     6

static UniversalMesh mesh;
static bool          _meshReady = false;

// Node mode: coordinator tracking
static uint8_t  _coordinatorMac[6]   = {};
static bool     _coordinatorFound    = false;
static uint32_t _coordinatorLastSeen = 0;

// Coordinator mode: track announcing nodes
struct NodeEntry { uint8_t mac[6]; uint32_t lastSeen; };
static NodeEntry _nodes[ESPNOW_MAX_PEERS] = {};
static int       _nodeCount = 0;

QueueHandle_t espnowDmrNetQueue  = nullptr;
QueueHandle_t espnowPocsagQueue  = nullptr;

extern bool espnowPocsagEnabled;

static void updateNodeEntry(const uint8_t* mac) {
  for (int i = 0; i < _nodeCount; i++) {
    if (memcmp(_nodes[i].mac, mac, 6) == 0) {
      _nodes[i].lastSeen = millis();
      return;
    }
  }
  if (_nodeCount < ESPNOW_MAX_PEERS) {
    memcpy(_nodes[_nodeCount].mac, mac, 6);
    _nodes[_nodeCount].lastSeen = millis();
    _nodeCount++;
  }
}

// ── Mesh receive callback ──────────────────────────────────────────────────
static void onMeshReceive(MeshPacket* packet, uint8_t* senderMac) {
  // Node mode: coordinator found via PONG with role=0x01
  if (packet->type == MESH_TYPE_PONG &&
      packet->payloadLen > 0 && packet->payload[0] == 0x01 &&
      !_coordinatorFound) {
    memcpy(_coordinatorMac, packet->srcMac, 6);
    _coordinatorFound    = true;
    _coordinatorLastSeen = millis();
    char mac[18];
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
             _coordinatorMac[0], _coordinatorMac[1], _coordinatorMac[2],
             _coordinatorMac[3], _coordinatorMac[4], _coordinatorMac[5]);
    addLogMessage(String("[MESH] Coordinator found: ") + mac);
    return;
  }

  // Node mode: update coordinator last-seen on any packet from coordinator
  if (_coordinatorFound && memcmp(packet->srcMac, _coordinatorMac, 6) == 0) {
    _coordinatorLastSeen = millis();
  }

  if (packet->type != MESH_TYPE_DATA) return;

  // 0x02 Raw Hex → DMR: payload is raw DMRD bytes
  if (packet->appId == MESH_APP_RAW) {
    if (!dmrServerEspNow || espnowDmrNetQueue == nullptr) return;
    if (packet->payloadLen < 1) return;
    EspNowDmrNetPacket pkt = {};
    pkt.type = ESPNOW_TYPE_DMR_NET;
    pkt.len  = packet->payloadLen;
    if (pkt.len > 60) pkt.len = 60;
    memcpy(pkt.data, packet->payload, pkt.len);
    xQueueSend(espnowDmrNetQueue, &pkt, 0);
  }

  // 0x01 Text → POCSAG: "P:<ric>,<func>,<message>"
  else if (packet->appId == MESH_APP_TEXT) {
    if (!pocsagServerEspNow || espnowPocsagQueue == nullptr) return;
    if (packet->payloadLen < 4) return;
    char buf[65] = {};
    int len = packet->payloadLen > 64 ? 64 : packet->payloadLen;
    memcpy(buf, packet->payload, len);
    if (buf[0] != 'P' || buf[1] != ':') return;
    EspNowPocsagPacket pkt = {};
    pkt.type = ESPNOW_TYPE_POCSAG;
    unsigned long ric = 0;
    unsigned int  func = 0;
    int parsed = sscanf(buf + 2, "%lu,%u,", &ric, &func);
    if (parsed < 2) return;
    pkt.ric        = (uint32_t)ric;
    pkt.functional = (uint8_t)func;
    // Find message start: skip "P:<ric>,<func>,"
    const char* msgPtr = strchr(buf + 2, ',');
    if (msgPtr) msgPtr = strchr(msgPtr + 1, ',');
    if (msgPtr) {
      msgPtr++;
      strncpy(pkt.message, msgPtr, POCSAG_MSG_MAX_LEN);
      pkt.message[POCSAG_MSG_MAX_LEN] = '\0';
    }
    xQueueSend(espnowPocsagQueue, &pkt, 0);
  }

  else if (packet->appId == MESH_APP_HEARTBEAT) {
    if (espnowSenderEnabled) updateNodeEntry(packet->srcMac);
  }
  else if (packet->appId == MESH_APP_ANNOUNCE) {
    if (espnowSenderEnabled) {
      updateNodeEntry(packet->srcMac);
      if (espnowDebug && packet->payloadLen > 0) {
        char name[65] = {};
        int len = packet->payloadLen > 64 ? 64 : packet->payloadLen;
        memcpy(name, packet->payload, len);
        addLogMessage(String("[MESH] Node: ") + name);
      }
    }
  }
}

// ── Background mesh task ───────────────────────────────────────────────────
static void espnowMeshTask(void* param) {
  uint8_t tmpMac[6];
  int waitMs = 0;
  while (esp_wifi_get_mac(WIFI_IF_STA, tmpMac) != ESP_OK && waitMs < 10000) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    waitMs += 100;
  }
  if (waitMs >= 10000) {
    addLogMessage("[MESH] WiFi driver not ready — aborting");
    vTaskDelete(nullptr);
    return;
  }

  bool isSender = espnowSenderEnabled;
  if (!mesh.begin(ESPNOW_MESH_CHANNEL, isSender)) {
    addLogMessage("[MESH] Init failed");
    vTaskDelete(nullptr);
    return;
  }
  mesh.onReceive(onMeshReceive);
  _meshReady = true;

  String nodeName = userCallsign;
  if (userDmrSsid > 0) nodeName += "-" + String(userDmrSsid);

  uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  if (isSender) {
    addLogMessage("[MESH] Ready — coordinator (" + nodeName + ")");
    mesh.send(broadcast, MESH_TYPE_DATA, MESH_APP_ANNOUNCE,
              (const uint8_t*)nodeName.c_str(), nodeName.length(), 4);
  } else {
    addLogMessage("[MESH] Ready — node (" + nodeName + "), searching for coordinator...");
    mesh.send(broadcast, MESH_TYPE_PING, 0x00, nullptr, 0, 4);
  }

  unsigned long lastHeartbeat = 0;
  unsigned long lastRetry     = 0;
  unsigned long lastAnnounce  = 0;

  for (;;) {
    mesh.update();
    unsigned long now = millis();

    if (isSender) {
      // Coordinator: periodic announce so nodes can refresh their last-seen
      if (now - lastAnnounce >= ANNOUNCE_INTERVAL) {
        lastAnnounce = now;
        mesh.send(broadcast, MESH_TYPE_DATA, MESH_APP_ANNOUNCE,
                  (const uint8_t*)nodeName.c_str(), nodeName.length(), 4);
      }
    } else if (dmrServerEspNow || pocsagServerEspNow) {
      // Node: retry coordinator discovery every 10 s until found
      if (!_coordinatorFound && now - lastRetry >= 10000) {
        lastRetry = now;
        mesh.send(broadcast, MESH_TYPE_PING, 0x00, nullptr, 0, 4);
        addLogMessage("[MESH] Searching for coordinator...");
      }
      // Node: heartbeat + re-announce once coordinator is known
      if (_coordinatorFound && now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
        lastHeartbeat = now;
        uint8_t hb = 0x01;
        mesh.send(_coordinatorMac, MESH_TYPE_DATA, MESH_APP_HEARTBEAT, &hb, 1, 4);
        mesh.send(_coordinatorMac, MESH_TYPE_DATA, MESH_APP_ANNOUNCE,
                  (const uint8_t*)nodeName.c_str(), nodeName.length(), 4);
      }
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ── Public API ─────────────────────────────────────────────────────────────

void initEspNow() {
  espnowDmrNetQueue = xQueueCreate(8, sizeof(EspNowDmrNetPacket));
  if (!espnowDmrNetQueue) {
    addLogMessage("[MESH] Failed to create DMR queue");
    return;
  }
  espnowPocsagQueue = xQueueCreate(8, sizeof(EspNowPocsagPacket));
  if (!espnowPocsagQueue) {
    addLogMessage("[MESH] Failed to create POCSAG queue");
    return;
  }
  xTaskCreatePinnedToCore(espnowMeshTask, "espnow_mesh", 4096, nullptr, 1, nullptr, 0);
}

void espnowSendDmrNetPacket(const uint8_t* dmrdPacket, uint8_t len) {
  if (!_meshReady || !espnowSenderEnabled) return;
  if (len > 60) len = 60;
  // appId 0x02 Raw Hex: raw DMRD bytes
  uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  mesh.send(broadcast, MESH_TYPE_DATA, MESH_APP_RAW, dmrdPacket, len, 4);
}

void espnowSendPocsagPacket(uint32_t ric, uint8_t functional, const String& message) {
  if (!_meshReady || !espnowSenderEnabled || !espnowPocsagEnabled) return;
  // appId 0x01 Text: "P:<ric>,<func>,<message>"
  char buf[64] = {};
  String msg = message;
  if (msg.length() > MESH_POCSAG_MSG_MAX) msg = msg.substring(0, MESH_POCSAG_MSG_MAX);
  snprintf(buf, sizeof(buf), "P:%lu,%u,%s", (unsigned long)ric, (unsigned int)functional, msg.c_str());
  uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  mesh.send(broadcast, MESH_TYPE_DATA, MESH_APP_TEXT, (const uint8_t*)buf, (uint8_t)strlen(buf), 4);
}

#endif  // ESPNOW_SENDER

// ── Mesh status JSON ─────────────────────────────────────────────────────────
// Always compiled. [{mac, status}, ...] × ESPNOW_MAX_PEERS
// Coordinator mode: shows announced nodes with last-seen.
// Node mode: shows coordinator connection status.
String espnowGetPeerStatusJson() {
  #if ESPNOW_SENDER
  String json = "[";
  if (espnowSenderEnabled) {
    // Add coordinator (this device) MAC as first entry
    String myMac = WiFi.macAddress();
    json += "{\"mac\":\"" + myMac + "\",\"status\":\"coordinator\"}";
    for (int i = 0; i < ESPNOW_MAX_PEERS - 1; i++) {
      json += ",";
      if (i < _nodeCount) {
        char mac[18];
        snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                 _nodes[i].mac[0], _nodes[i].mac[1], _nodes[i].mac[2],
                 _nodes[i].mac[3], _nodes[i].mac[4], _nodes[i].mac[5]);
        bool recent = (millis() - _nodes[i].lastSeen) < 120000;
        json += "{\"mac\":\"" + String(mac) + "\",\"status\":\"" + (recent ? "ok" : "idle") + "\"}";
      } else {
        json += "{\"mac\":\"\",\"status\":\"none\"}";
      }
    }
  } else {
    if (_coordinatorFound) {
      char mac[18];
      snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
               _coordinatorMac[0], _coordinatorMac[1], _coordinatorMac[2],
               _coordinatorMac[3], _coordinatorMac[4], _coordinatorMac[5]);
      bool recent = (millis() - _coordinatorLastSeen) < 120000;
      json += "{\"mac\":\"" + String(mac) + "\",\"status\":\"" + (recent ? "ok" : "idle") + "\"}";
    } else {
      json += "{\"mac\":\"searching\",\"status\":\"idle\"}";
    }
    for (int i = 1; i < ESPNOW_MAX_PEERS; i++) {
      json += ",{\"mac\":\"\",\"status\":\"none\"}";
    }
  }
  json += "]";
  return json;
  #else
  return "[]";
  #endif
}
