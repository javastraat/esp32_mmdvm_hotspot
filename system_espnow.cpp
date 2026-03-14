/*
 * system_espnow.cpp - ESP-NOW DMR frame relay
 *
 * Sender: forwards raw BrandMeister DMRD Homebrew packets over ESP-NOW.
 * Receiver: receives those packets via callback, enqueues them to
 *           espnowDmrNetQueue for the DMR task to drain each loop iteration.
 *
 * The remote node (same firmware) processes the packets exactly as if they
 * arrived from BrandMeister — full callsign lookup, OLED, web UI, modem TX.
 *
 * Sender hook in mmdvm_dmr.cpp:
 *   handleDMRNetwork() → after validating DMRD packet → espnowSendDmrNetPacket()
 *
 * Receiver injection in mmdvm_dmr.cpp:
 *   handleDMRNetwork() → drains espnowDmrNetQueue → processDMRDPacket()
 */

#include "system/system_espnow.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include "system/system_logger.h"
#include "system/system_eth.h"
#include "system/system_wifi.h"

// ── Discovery responder ─────────────────────────────────────────────────────
// Listens on UDP port 3491 for {"type":"ESPNOW_DISCOVER"} broadcast pings
// (sent by the ESP-NOW transmitter sketch) and replies with this device's
// MAC address + callsign so the transmitter can auto-populate its peer list.

extern String   userCallsign;
extern uint8_t  userDmrSsid;
extern bool     dmrServerEspNow;
extern bool     pocsagServerEspNow;

#define ESPNOW_DISCOVERY_PORT 3491

static void espnowDiscoveryTask(void* param) {
  // Wait until any network interface is up
  while (WiFi.status() != WL_CONNECTED && !ethConnected && !softAPActive) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }

  WiFiUDP udp;
  if (!udp.begin(ESPNOW_DISCOVERY_PORT)) {
    addLogMessage("[ESP-NOW] Discovery: failed to bind UDP port " + String(ESPNOW_DISCOVERY_PORT));
    vTaskDelete(nullptr);
    return;
  }
  addLogMessage("[ESP-NOW] Discovery responder listening on UDP port " + String(ESPNOW_DISCOVERY_PORT));

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

#if ESPNOW_SENDER

#include <esp_now.h>
#include <esp_wifi.h>
#include "system/system_logger.h"

#define ESPNOW_MAX_PEERS 6
static uint8_t _peerMacs[ESPNOW_MAX_PEERS][6] = {};
static int     _peerCount = 0;
static bool    _ready     = false;

// ── Per-peer send status ───────────────────────────────────────────────────
struct PeerStatus {
  uint32_t lastSentMs;   // millis() when last frame was sent to this peer
  bool     lastOk;       // was the last send ACK'd?
  bool     everSent;     // has anything been sent to this peer yet?
};
static PeerStatus _peerStatus[ESPNOW_MAX_PEERS] = {};

// Ring buffer to map async send callbacks back to their peer index.
// pushSendIdx() is called just before esp_now_send(); onSendResult()
// calls popSendIdx() to get the matching peer slot.
#define SEND_QUEUE_SIZE 16
static volatile uint8_t _sendIdxQueue[SEND_QUEUE_SIZE] = {};
static volatile uint8_t _sendQHead = 0;
static volatile uint8_t _sendQTail = 0;

static void pushSendIdx(int idx) {
  _sendIdxQueue[_sendQHead % SEND_QUEUE_SIZE] = (uint8_t)idx;
  _sendQHead = (_sendQHead + 1) % SEND_QUEUE_SIZE;
}
static int popSendIdx() {
  if (_sendQHead == _sendQTail) return -1;
  int idx = _sendIdxQueue[_sendQTail % SEND_QUEUE_SIZE];
  _sendQTail = (_sendQTail + 1) % SEND_QUEUE_SIZE;
  return idx;
}

QueueHandle_t espnowDmrNetQueue  = nullptr;
QueueHandle_t espnowPocsagQueue  = nullptr;

// -------------------------------------------------------
// Parse "AA:BB:CC:DD:EE:FF" into 6-byte array.
// Returns true on success.
// -------------------------------------------------------
static bool parseMacString(const String& macStr, uint8_t* out) {
  if (macStr.length() < 17) return false;
  for (int i = 0; i < 6; i++) {
    out[i] = (uint8_t)strtoul(macStr.substring(i * 3, i * 3 + 2).c_str(), nullptr, 16);
  }
  return true;
}

// -------------------------------------------------------
// Parse comma-separated MAC list "AA:BB:CC:DD:EE:FF,11:22:33:44:55:66"
// Returns number of successfully parsed MACs.
// -------------------------------------------------------
static int parseMacList(const String& macList, uint8_t out[][6], int maxPeers) {
  int count = 0;
  int start = 0;
  while (count < maxPeers) {
    int comma = macList.indexOf(',', start);
    String token = (comma < 0) ? macList.substring(start) : macList.substring(start, comma);
    token.trim();
    if (token.length() == 17 && parseMacString(token, out[count])) {
      count++;
    }
    if (comma < 0) break;
    start = comma + 1;
  }
  return count;
}

// -------------------------------------------------------
// Send callback (sender only) — fires after frame handed to radio driver
// -------------------------------------------------------
static void onSendResult(const wifi_tx_info_t* info, esp_now_send_status_t status) {
  bool ok = (status == ESP_NOW_SEND_SUCCESS);
  int idx = popSendIdx();
  if (idx >= 0 && idx < _peerCount) {
    _peerStatus[idx].lastSentMs = millis();
    _peerStatus[idx].lastOk     = ok;
    _peerStatus[idx].everSent   = true;
  }
  if (espnowDebug && !ok) {
    addLogMessage("[ESP-NOW] No ACK from peer");
  }
}

// -------------------------------------------------------
// Receive callback — runs in WiFi task context on BOTH sender and receiver.
// Enqueues incoming DMRD packets for the DMR task to drain.
// -------------------------------------------------------
static void onReceive(const esp_now_recv_info_t* info,
                      const uint8_t* inData, int dataLen) {
  if (dataLen < 2) return;

  if (inData[0] == ESPNOW_TYPE_DMR_NET) {
    if (espnowDmrNetQueue == nullptr) return;
    EspNowDmrNetPacket pkt = {};
    memcpy(&pkt, inData, (dataLen < (int)sizeof(pkt)) ? dataLen : sizeof(pkt));
    xQueueSend(espnowDmrNetQueue, &pkt, 0);
  }
  else if (inData[0] == ESPNOW_TYPE_POCSAG) {
    if (espnowPocsagQueue == nullptr) return;
    EspNowPocsagPacket pkt = {};
    memcpy(&pkt, inData, (dataLen < (int)sizeof(pkt)) ? dataLen : sizeof(pkt));
    xQueueSend(espnowPocsagQueue, &pkt, 0);
  }
}

// -------------------------------------------------------
// initEspNow() — call from setup() when sender OR receiver is enabled
// -------------------------------------------------------
void initEspNow() {
  // Create the receive queue (used by both sender and receiver side)
  espnowDmrNetQueue = xQueueCreate(8, sizeof(EspNowDmrNetPacket));
  if (!espnowDmrNetQueue) {
    addLogMessage("[ESP-NOW] Failed to create DMR receive queue");
    return;
  }
  espnowPocsagQueue = xQueueCreate(8, sizeof(EspNowPocsagPacket));
  if (!espnowPocsagQueue) {
    addLogMessage("[ESP-NOW] Failed to create POCSAG receive queue");
    return;
  }

  // Parse comma-separated receiver MACs (sender side needs this)
  if (espnowSenderEnabled) {
    _peerCount = parseMacList(espnowReceiverMac, _peerMacs, ESPNOW_MAX_PEERS);
    if (_peerCount == 0) {
      addLogMessage("[ESP-NOW] No valid receiver MACs — check espnowReceiverMac setting");
    }
  }

  // WiFi task starts asynchronously — wait until the driver is up before
  // calling esp_now_init(), otherwise it crashes (LoadProhibited).
  uint8_t tmpMac[6];
  int waitMs = 0;
  while (esp_wifi_get_mac(WIFI_IF_STA, tmpMac) != ESP_OK && waitMs < 10000) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    waitMs += 100;
  }
  if (waitMs >= 10000) {
    addLogMessage("[ESP-NOW] WiFi driver not ready after 10s — skipping init");
    return;
  }

  if (esp_now_init() != ESP_OK) {
    addLogMessage("[ESP-NOW] Init failed");
    return;
  }

  // Register receive callback — needed on both sender and receiver
  esp_now_register_recv_cb(onReceive);

  // Register send callback and add all peers — sender only
  if (espnowSenderEnabled) {
    esp_now_register_send_cb(onSendResult);

    int added = 0;
    for (int i = 0; i < _peerCount; i++) {
      esp_now_peer_info_t peer = {};
      memcpy(peer.peer_addr, _peerMacs[i], 6);
      peer.channel = 0;
      peer.encrypt = false;
      if (esp_now_add_peer(&peer) == ESP_OK) {
        added++;
      } else {
        addLogMessage(String("[ESP-NOW] Failed to add peer ") + (i + 1));
      }
    }

    addLogMessage(String("[ESP-NOW] Sender ready — ") + added + "/" + _peerCount + " peer(s): " + espnowReceiverMac);
  }



  _ready = true;
}

// -------------------------------------------------------
// espnowSendDmrNetPacket() — called from handleDMRNetwork() (sender side)
// Forwards the raw DMRD Homebrew packet to the peer over ESP-NOW.
// -------------------------------------------------------
void espnowSendDmrNetPacket(const uint8_t* dmrdPacket, uint8_t len) {
  if (!_ready || !espnowSenderEnabled) return;
  if (len > 60) len = 60;

  EspNowDmrNetPacket pkt = {};
  pkt.type = ESPNOW_TYPE_DMR_NET;
  pkt.len  = len;
  memcpy(pkt.data, dmrdPacket, len);

  for (int i = 0; i < _peerCount; i++) {
    pushSendIdx(i);
    esp_now_send(_peerMacs[i], (uint8_t*)&pkt, sizeof(pkt));
  }
}

// -------------------------------------------------------
// espnowSendPocsagPacket() — called from DAPNET task (sender side)
// Forwards a received DAPNET/POCSAG page to the peer over ESP-NOW.
// -------------------------------------------------------
extern bool espnowPocsagEnabled;

void espnowSendPocsagPacket(uint32_t ric, uint8_t functional, const String& message) {
  if (!_ready || !espnowSenderEnabled || !espnowPocsagEnabled) return;

  EspNowPocsagPacket pkt = {};
  pkt.type       = ESPNOW_TYPE_POCSAG;
  pkt.ric        = ric;
  pkt.functional = functional;
  strncpy(pkt.message, message.c_str(), POCSAG_MSG_MAX_LEN);
  pkt.message[POCSAG_MSG_MAX_LEN] = '\0';

  for (int i = 0; i < _peerCount; i++) {
    pushSendIdx(i);
    esp_now_send(_peerMacs[i], (uint8_t*)&pkt, sizeof(pkt));
  }
}

#endif  // ESPNOW_SENDER

// -------------------------------------------------------
// espnowGetPeerStatusJson() — returns JSON array with per-peer ACK status.
// Always compiled so the web handler can call it unconditionally.
// -------------------------------------------------------
String espnowGetPeerStatusJson() {
#if ESPNOW_SENDER
  String json = "[";
  for (int i = 0; i < ESPNOW_MAX_PEERS; i++) {
    if (i > 0) json += ",";
    if (i < _peerCount) {
      char mac[18];
      snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
               _peerMacs[i][0], _peerMacs[i][1], _peerMacs[i][2],
               _peerMacs[i][3], _peerMacs[i][4], _peerMacs[i][5]);
      const char* status;
      if (!_peerStatus[i].everSent) {
        status = "idle";
      } else if ((millis() - _peerStatus[i].lastSentMs) > 30000) {
        status = "idle";
      } else if (_peerStatus[i].lastOk) {
        status = "ok";
      } else {
        status = "fail";
      }
      json += "{\"mac\":\"" + String(mac) + "\",\"status\":\"" + status + "\"}";
    } else {
      json += "{\"mac\":\"\",\"status\":\"none\"}";
    }
  }
  json += "]";
  return json;
#else
  return "[]";
#endif
}
