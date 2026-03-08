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

static uint8_t _peerMac[6] = {};
static bool    _ready       = false;

QueueHandle_t espnowDmrNetQueue = nullptr;

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
  if (dataLen < 2 || inData[0] != ESPNOW_TYPE_DMR_NET) return;
  if (espnowDmrNetQueue == nullptr) return;

  EspNowDmrNetPacket pkt = {};
  // The incoming bytes ARE the packed struct — copy directly
  memcpy(&pkt, inData, (dataLen < (int)sizeof(pkt)) ? dataLen : sizeof(pkt));

  // Non-blocking enqueue — drop if DMR task is behind (queue full)
  xQueueSend(espnowDmrNetQueue, &pkt, 0);
}

// -------------------------------------------------------
// initEspNow() — call from setup() when sender OR receiver is enabled
// -------------------------------------------------------
void initEspNow() {
  // Create the receive queue (used by both sender and receiver side)
  espnowDmrNetQueue = xQueueCreate(8, sizeof(EspNowDmrNetPacket));
  if (!espnowDmrNetQueue) {
    addLogMessage("[ESP-NOW] Failed to create receive queue");
    return;
  }

  // Parse receiver MAC from runtime setting (sender side needs this)
  if (espnowSenderEnabled) {
    if (!parseMacString(espnowReceiverMac, _peerMac)) {
      addLogMessage("[ESP-NOW] Invalid receiver MAC — check espnowReceiverMac setting");
      // Continue — receiver-only mode still works without a peer MAC
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

  // Register send callback and add peer — sender only
  if (espnowSenderEnabled) {
    esp_now_register_send_cb(onSendResult);

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, _peerMac, 6);
    peer.channel = 0;      // follow current WiFi channel
    peer.encrypt = false;

    if (esp_now_add_peer(&peer) != ESP_OK) {
      addLogMessage("[ESP-NOW] Failed to add peer — check espnowReceiverMac setting");
      return;
    }

    addLogMessage(String("[ESP-NOW] Sender ready — peer: ") + espnowReceiverMac);
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

  esp_now_send(_peerMac, (uint8_t*)&pkt, sizeof(pkt));
}

#endif  // ESPNOW_SENDER
