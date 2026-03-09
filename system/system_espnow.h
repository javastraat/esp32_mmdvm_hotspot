/*
 * system_espnow.h - ESP-NOW DMR frame relay
 *
 * Forwards raw BrandMeister DMRD UDP packets over ESP-NOW to a second
 * ESP32+modem running the same firmware. The receiver processes them
 * exactly as if they arrived from BrandMeister directly — full callsign
 * lookup, OLED display, web UI, and modem TX all work on the remote node.
 *
 * Runtime settings loaded from NVS. Compile-time defaults in include/config.h.
 *
 * Packet flow:
 *   BrandMeister → sender UDP → [ESP-NOW] → receiver queue → receiver DMR task → modem TX
 */

#ifndef SYSTEM_ESPNOW_H
#define SYSTEM_ESPNOW_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "../include/config.h"
#include "../mmdvm/mmdvm_pocsag.h"

// Runtime variables — defined in esp32_mmdvm_hotspot.ino, loaded from NVS
extern bool   espnowSenderEnabled;
extern String espnowReceiverMac;   // "XX:XX:XX:XX:XX:XX"
extern bool   espnowDebug;
extern bool   espnowDmrEnabled;
extern bool   espnowPocsagEnabled;

#if ESPNOW_SENDER

// ── Packet types ───────────────────────────────────────────────────────────
#define ESPNOW_TYPE_DMR_NET  0x10   // Raw DMRD Homebrew protocol packet
#define ESPNOW_TYPE_POCSAG   0x11   // POCSAG page (RIC + functional + message)

// DMR packet — 62 bytes total, must be identical on sender and receiver.
// data[] holds the raw DMRD UDP payload (header + 33-byte DMR frame, ~53-55 bytes).
struct __attribute__((packed)) EspNowDmrNetPacket {
  uint8_t type;       // ESPNOW_TYPE_DMR_NET
  uint8_t len;        // number of valid bytes in data[] (typically 53-55)
  uint8_t data[60];   // raw DMRD Homebrew packet bytes
};

// POCSAG packet — 87 bytes total.
struct __attribute__((packed)) EspNowPocsagPacket {
  uint8_t  type;                        // ESPNOW_TYPE_POCSAG
  uint32_t ric;
  uint8_t  functional;
  char     message[POCSAG_MSG_MAX_LEN + 1]; // 81 bytes
};

// Queues for received ESP-NOW packets — polled by DMR/DAPNET tasks
extern QueueHandle_t espnowDmrNetQueue;
extern QueueHandle_t espnowPocsagQueue;

// ── Public API ─────────────────────────────────────────────────────────────
void initEspNow();
void espnowSendDmrNetPacket(const uint8_t* dmrdPacket, uint8_t len);
void espnowSendPocsagPacket(uint32_t ric, uint8_t functional, const String& message);

#endif  // ESPNOW_SENDER

// ── Discovery responder (always compiled — lets transmitters find this device) ──
void initEspNowDiscovery();

#endif  // SYSTEM_ESPNOW_H
