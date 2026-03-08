/*
 * system_espnow.h - ESP-NOW DMR frame relay
 *
 * Mirrors every incoming DMR frame to a second ESP32+modem over ESP-NOW.
 * Runtime settings are stored in NVS and exposed as global vars in the .ino.
 * Compile-time defaults live in include/config.h.
 */

#ifndef SYSTEM_ESPNOW_H
#define SYSTEM_ESPNOW_H

#include <Arduino.h>
#include "../include/config.h"

// Runtime variables — defined in esp32_mmdvm_hotspot.ino, loaded from NVS
extern bool   espnowSenderEnabled;
extern bool   espnowReceiverEnabled;
extern String espnowReceiverMac;   // "XX:XX:XX:XX:XX:XX"
extern bool   espnowDebug;
extern bool   espnowDmrEnabled;
extern bool   espnowPocsagEnabled;

#if ESPNOW_SENDER

// Packet type bytes (must match receiver firmware)
#define ESPNOW_TYPE_START  0x01   // Begin DMR transmission on receiver
#define ESPNOW_TYPE_FRAME  0x02   // DMR frame data
#define ESPNOW_TYPE_END    0x03   // End DMR transmission on receiver

// Binary packet — 36 bytes total, must be identical on sender and receiver
struct __attribute__((packed)) EspNowDmrPacket {
  uint8_t type;       // ESPNOW_TYPE_START / FRAME / END
  uint8_t slot;       // DMR slot (1 or 2)
  uint8_t data[34];   // modem data: [0]=ctrl byte, [1..33]=33-byte DMR frame
                      // only populated for ESPNOW_TYPE_FRAME
};

void initEspNow();
void espnowSendDmrStart(uint8_t slot);
void espnowSendDmrFrame(uint8_t slot, uint8_t* modemData34);
void espnowSendDmrEnd(uint8_t slot);

#endif  // ESPNOW_SENDER
#endif  // SYSTEM_ESPNOW_H
