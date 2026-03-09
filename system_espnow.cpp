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

#if ESPNOW_SENDER

#include <esp_now.h>
#include <esp_wifi.h>
#include "system/system_logger.h"

#define ESPNOW_MAX_PEERS 6
static uint8_t _peerMacs[ESPNOW_MAX_PEERS][6] = {};
static int     _peerCount = 0;
static bool    _ready     = false;

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
  if (espnowDebug && status != ESP_NOW_SEND_SUCCESS) {
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
    esp_now_send(_peerMacs[i], (uint8_t*)&pkt, sizeof(pkt));
  }
}

#endif  // ESPNOW_SENDER
