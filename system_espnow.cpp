/*
 * system_espnow.cpp - ESP-NOW DMR frame relay (sender side)
 *
 * Mirrors every incoming Net→RF DMR frame to a second ESP32+modem over ESP-NOW.
 * The receiver feeds the frames into its local MMDVM modem so both devices TX.
 *
 * Hook points in mmdvm_dmr.cpp (all guarded by #if ESPNOW_SENDER):
 *   writeDMRStart(true)             -> espnowSendDmrStart(slot)
 *   sendMMDVMCommand(frame.data,34) -> espnowSendDmrFrame(slot, frame.data)
 *   writeDMRStart(false)            -> espnowSendDmrEnd(slot)
 *
 * Runtime settings loaded from NVS via esp32_mmdvm_hotspot.ino:
 *   espnowSenderEnabled, espnowReceiverMac, espnowDebug
 */

#include "system/system_espnow.h"

#if ESPNOW_SENDER

#include <esp_now.h>
#include <esp_wifi.h>
#include "system/system_logger.h"

static uint8_t _peerMac[6] = {};
static bool    _ready       = false;

// Parse "AA:BB:CC:DD:EE:FF" into 6-byte array. Returns true on success.
static bool parseMacString(const String& macStr, uint8_t* out) {
  if (macStr.length() < 17) return false;
  for (int i = 0; i < 6; i++) {
    out[i] = (uint8_t)strtoul(macStr.substring(i * 3, i * 3 + 2).c_str(), nullptr, 16);
  }
  return true;
}

// Send callback — fires after frame handed to radio driver
static void onSendResult(const wifi_tx_info_t* info, esp_now_send_status_t status) {
  if (espnowDebug && status != ESP_NOW_SEND_SUCCESS) {
    addLogMessage("[ESP-NOW] No ACK from peer");
  }
}

// -------------------------------------------------------
// initEspNow() — call from setup() after WiFi is up
// -------------------------------------------------------
void initEspNow() {
  // Parse receiver MAC from runtime setting
  if (!parseMacString(espnowReceiverMac, _peerMac)) {
    addLogMessage("[ESP-NOW] Invalid receiver MAC — check espnowReceiverMac setting");
    return;
  }

  // WiFi task starts asynchronously — wait until the WiFi driver is running
  // before calling esp_now_init(), otherwise it crashes (LoadProhibited).
  // Poll esp_wifi_get_mac(): returns ESP_ERR_WIFI_NOT_INIT until driver is up.
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

  esp_now_register_send_cb(onSendResult);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, _peerMac, 6);
  peer.channel = 0;      // follow current WiFi channel
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    addLogMessage("[ESP-NOW] Failed to add peer — check espnowReceiverMac setting");
    return;
  }

  _ready = true;
  addLogMessage(String("[ESP-NOW] Ready — peer: ") + espnowReceiverMac);
}

// -------------------------------------------------------
// Internal send helper
// -------------------------------------------------------
static void sendPacket(uint8_t type, uint8_t slot, uint8_t* data, uint8_t dataLen) {
  if (!_ready) return;

  EspNowDmrPacket pkt = {};
  pkt.type = type;
  pkt.slot = slot;
  if (data && dataLen > 0) {
    memcpy(pkt.data, data, dataLen < 34 ? dataLen : 34);
  }

  esp_now_send(_peerMac, (uint8_t*)&pkt, sizeof(pkt));
}

// -------------------------------------------------------
// Public API — called from the three hook points in mmdvm_dmr.cpp
// -------------------------------------------------------
void espnowSendDmrStart(uint8_t slot) {
  sendPacket(ESPNOW_TYPE_START, slot, nullptr, 0);
}

void espnowSendDmrFrame(uint8_t slot, uint8_t* modemData34) {
  sendPacket(ESPNOW_TYPE_FRAME, slot, modemData34, 34);
}

void espnowSendDmrEnd(uint8_t slot) {
  sendPacket(ESPNOW_TYPE_END, slot, nullptr, 0);
}

#endif  // ESPNOW_SENDER
