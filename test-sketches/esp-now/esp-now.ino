/*
 * ESP-NOW DMR Gateway Monitor
 *
 * Matches the updated firmware: raw BrandMeister DMRD Homebrew packets
 * are forwarded over ESP-NOW as EspNowDmrNetPacket (type 0x10).
 *
 * RECEIVER: decodes every incoming packet and prints the DMRD fields:
 *           srcId, dstId, slot, group/unit, seqNo, stream, DMR frame bytes.
 *
 * SENDER:   sends dummy DMRD packets so you can verify the link without
 *           a real hotspot.
 *
 * Role selected in config.h:  #define ROLE_SENDER  or  #define ROLE_RECEIVER
 */

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "config.h"

// ============================================================
// Packet definition — MUST match system/system_espnow.h exactly
// ============================================================
#define ESPNOW_TYPE_DMR_NET  0x10   // Raw DMRD Homebrew protocol packet

struct __attribute__((packed)) EspNowDmrNetPacket {
  uint8_t type;       // ESPNOW_TYPE_DMR_NET
  uint8_t len;        // number of valid bytes in data[] (typically 53-55)
  uint8_t data[60];   // raw DMRD Homebrew packet bytes
};

// ============================================================
// Sanity check
// ============================================================
#if defined(ROLE_SENDER) && defined(ROLE_RECEIVER)
  #error "Define ROLE_SENDER *or* ROLE_RECEIVER in config.h, not both."
#endif
#if !defined(ROLE_SENDER) && !defined(ROLE_RECEIVER)
  #error "Define ROLE_SENDER or ROLE_RECEIVER in config.h."
#endif

// ============================================================
// DMRD packet helpers
// ============================================================

// Parse the DMRD Homebrew header and print decoded fields + first 8 DMR frame bytes.
// Packet layout:
//   [0-3]   "DMRD"
//   [4]     sequence number
//   [5-7]   srcId  (3 bytes BE)
//   [8-10]  dstId  (3 bytes BE)
//   [11-14] repeater ID (4 bytes BE)
//   [15]    flags  (bit7 = slot2, bit6 = group call)
//   [16-19] stream ID (4 bytes)
//   [20-52] 33-byte DMR frame
static void printDmrdPacket(const uint8_t* pkt, uint8_t len, const char* srcMac,
                             uint32_t totalRx, uint32_t frameCount)
{
  if (len < 21) {
    Serial.printf("[RX] DMRD too short (%d bytes)\n", len);
    return;
  }

  uint8_t  seqNo   = pkt[4];
  uint32_t srcId   = ((uint32_t)pkt[5] << 16) | ((uint32_t)pkt[6] << 8) | pkt[7];
  uint32_t dstId   = ((uint32_t)pkt[8] << 16) | ((uint32_t)pkt[9] << 8) | pkt[10];
  uint8_t  flags   = pkt[15];
  uint8_t  slot    = (flags & 0x80) ? 2 : 1;
  bool     isGroup = (flags & 0x40) != 0;

  // Stream ID as hex string
  char streamHex[9];
  snprintf(streamHex, sizeof(streamHex), "%02X%02X%02X%02X",
           pkt[16], pkt[17], pkt[18], pkt[19]);

  // First 8 bytes of DMR frame (offset 20)
  Serial.printf("[RX #%lu] src=%-8lu  dst=%s%-8lu  slot=%d  seq=%3d  stream=%s\n",
    totalRx, srcId,
    isGroup ? "TG" : "", dstId,
    slot, seqNo, streamHex);

  if (len >= 28) {
    Serial.printf("         DMR frame[0..7]: %02X %02X %02X %02X %02X %02X %02X %02X\n",
      pkt[20], pkt[21], pkt[22], pkt[23],
      pkt[24], pkt[25], pkt[26], pkt[27]);
  }
}

// ============================================================
// SENDER — sends dummy DMRD-formatted packets
// ============================================================
#ifdef ROLE_SENDER

static uint8_t peerMac[] = RECEIVER_MAC;
static bool    peerRegistered = false;
static uint8_t seqCounter = 0;

// Build a minimal valid-looking DMRD packet (55 bytes)
static uint8_t buildDmrdPacket(uint8_t* buf, uint32_t srcId, uint32_t dstId,
                                uint8_t slot, bool group, uint8_t seq)
{
  memcpy(buf, "DMRD", 4);
  buf[4] = seq;
  buf[5] = (srcId >> 16) & 0xFF;
  buf[6] = (srcId >>  8) & 0xFF;
  buf[7] =  srcId        & 0xFF;
  buf[8] = (dstId >> 16) & 0xFF;
  buf[9] = (dstId >>  8) & 0xFF;
  buf[10] =  dstId       & 0xFF;
  buf[11] = buf[12] = buf[13] = buf[14] = 0x00;   // repeater ID (unused in test)
  buf[15] = (slot == 2 ? 0x80 : 0x00) | (group ? 0x40 : 0x00);
  buf[16] = 0xAB; buf[17] = 0xCD; buf[18] = buf[19] = seq;   // stream ID
  // DMR frame: fill bytes 20-52 with counter pattern
  for (int i = 20; i < 55; i++) buf[i] = (uint8_t)((seq + i) & 0xFF);
  return 55;
}

static void sendDmrdPkt(uint32_t srcId, uint32_t dstId, uint8_t slot, bool group)
{
  uint8_t dmrd[60] = {};
  uint8_t dlen = buildDmrdPacket(dmrd, srcId, dstId, slot, group, ++seqCounter);

  EspNowDmrNetPacket pkt = {};
  pkt.type = ESPNOW_TYPE_DMR_NET;
  pkt.len  = dlen;
  memcpy(pkt.data, dmrd, dlen);

  esp_now_send(peerMac, (uint8_t*)&pkt, sizeof(pkt));
}

void onSendResult(const wifi_tx_info_t* info, esp_now_send_status_t status) {
  Serial.printf("  delivery: %s\n", status == ESP_NOW_SEND_SUCCESS ? "ACK" : "NO ACK");
}

void setupSender() {
  Serial.println("[ROLE] SENDER (dummy DMRD packets)");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[WiFi] Connecting to %s ", WIFI_SSID);
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 8000) {
    delay(250); Serial.print(".");
  }
  Serial.printf("\n[WiFi] %s\n",
    WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString().c_str() : "not connected — ESP-NOW still works");

  Serial.printf("[INFO] My MAC: %s\n", WiFi.macAddress().c_str());

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] Init FAILED — halting.");
    while (true) delay(1000);
  }
  esp_now_register_send_cb(onSendResult);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, peerMac, 6);
  peer.channel = 0;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("[ESP-NOW] Failed to add peer — check RECEIVER_MAC in config.h");
  } else {
    peerRegistered = true;
    Serial.printf("[ESP-NOW] Peer: %02X:%02X:%02X:%02X:%02X:%02X\n",
      peerMac[0], peerMac[1], peerMac[2], peerMac[3], peerMac[4], peerMac[5]);
  }
  Serial.println("[SENDER] Ready — sending fake DMRD packet every 2s\n");
}

void loopSender() {
  if (!peerRegistered) return;

  static unsigned long lastSend = 0;
  static uint8_t cycle = 0;
  if (millis() - lastSend < 2000) return;
  lastSend = millis();

  // Alternate between two fake calls
  uint32_t src  = (cycle % 2 == 0) ? 2620123 : 2620456;
  uint32_t dst  = 204;     // TG204
  uint8_t  slot = (cycle % 2) + 1;

  Serial.printf("\n[TX] DMRD  src=%-8lu  dst=TG%-6lu  slot=%d  seq=%d\n",
    src, dst, slot, seqCounter + 1);

  sendDmrdPkt(src, dst, slot, true);
  cycle++;
}

#endif  // ROLE_SENDER

// ============================================================
// RECEIVER — monitor for real hotspot ESP-NOW output
// ============================================================
#ifdef ROLE_RECEIVER

static uint32_t rxTotal      = 0;
static uint32_t callFrames   = 0;
static uint32_t callSrc      = 0;
static uint32_t callDst      = 0;
static uint8_t  callSlot     = 0;
static unsigned long callStart = 0;

void onReceive(const esp_now_recv_info_t* info, const uint8_t* inData, int inLen) {
  if (inLen < 2) return;

  uint8_t type = inData[0];

  if (type != ESPNOW_TYPE_DMR_NET) {
    Serial.printf("[RX] Unknown type 0x%02X (%d bytes)\n", type, inLen);
    return;
  }

  EspNowDmrNetPacket pkt;
  memcpy(&pkt, inData, (inLen < (int)sizeof(pkt)) ? inLen : sizeof(pkt));

  if (pkt.len < 21 || memcmp(pkt.data, "DMRD", 4) != 0) {
    Serial.printf("[RX] Bad DMRD payload (len=%d)\n", pkt.len);
    return;
  }

  rxTotal++;

  uint32_t srcId   = ((uint32_t)pkt.data[5] << 16) | ((uint32_t)pkt.data[6] << 8) | pkt.data[7];
  uint32_t dstId   = ((uint32_t)pkt.data[8] << 16) | ((uint32_t)pkt.data[9] << 8) | pkt.data[10];
  uint8_t  flags   = pkt.data[15];
  uint8_t  slot    = (flags & 0x80) ? 2 : 1;
  bool     isGroup = (flags & 0x40) != 0;
  uint8_t  seq     = pkt.data[4];

  bool isNewCall = (srcId != callSrc || dstId != callDst || slot != callSlot);

  if (isNewCall) {
    // Print summary of previous call if there was one
    if (callFrames > 0) {
      unsigned long dur = (millis() - callStart) / 1000;
      Serial.printf("[RX] ── END   src=%-8lu  dst=%s%-6lu  slot=%d  frames=%lu  dur=%lus\n\n",
        callSrc, callDst > 0 ? "TG" : "", callDst, callSlot, callFrames, dur);
    }
    // Print new call header
    callSrc   = srcId;
    callDst   = dstId;
    callSlot  = slot;
    callFrames = 0;
    callStart  = millis();
    Serial.printf("[RX] ══ NEW  src=%-8lu  dst=%s%-6lu  slot=%d  pkt#%lu\n",
      srcId, isGroup ? "TG" : "", dstId, slot, rxTotal);
  }

  callFrames++;

#if ESPNOW_DEBUG
  // Verbose: print every frame
  char streamHex[9];
  snprintf(streamHex, sizeof(streamHex), "%02X%02X%02X%02X",
    pkt.data[16], pkt.data[17], pkt.data[18], pkt.data[19]);
  Serial.printf("  [#%lu] seq=%3d  stream=%s  frame: %02X %02X %02X %02X %02X %02X %02X %02X\n",
    callFrames, seq, streamHex,
    pkt.data[20], pkt.data[21], pkt.data[22], pkt.data[23],
    pkt.data[24], pkt.data[25], pkt.data[26], pkt.data[27]);
#endif
}

void setupReceiver() {
  Serial.println("[ROLE] RECEIVER — monitoring real hotspot ESP-NOW output");
#if ESPNOW_DEBUG
  Serial.println("[MODE] Debug: ON — printing every frame");
#else
  Serial.println("[MODE] Debug: OFF — printing call start/end only  (set ESPNOW_DEBUG true in config.h for full frames)");
#endif

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] Init FAILED — halting.");
    while (true) delay(1000);
  }

  uint8_t macBytes[6];
  esp_wifi_get_mac(WIFI_IF_STA, macBytes);
  Serial.println("[INFO] My MAC (use as espnowReceiverMac in hotspot settings):");
  Serial.printf("       %02X:%02X:%02X:%02X:%02X:%02X\n",
    macBytes[0], macBytes[1], macBytes[2],
    macBytes[3], macBytes[4], macBytes[5]);

  esp_now_register_recv_cb(onReceive);
  Serial.println("\n[RECEIVER] Listening...\n");
}

void loopReceiver() {
  // Print running frame count for current call every 5s so screen stays alive
#if !ESPNOW_DEBUG
  static unsigned long lastPrint = 0;
  if (callFrames > 0 && millis() - lastPrint >= 5000) {
    lastPrint = millis();
    unsigned long dur = (millis() - callStart) / 1000;
    Serial.printf("[RX] ... src=%-8lu  frames=%lu  dur=%lus\n", callSrc, callFrames, dur);
  }
#endif
}

#endif  // ROLE_RECEIVER

// ============================================================
// Arduino entry points
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== ESP-NOW DMR Gateway Monitor ===");

#ifdef ROLE_SENDER
  setupSender();
#endif
#ifdef ROLE_RECEIVER
  setupReceiver();
#endif
}

void loop() {
#ifdef ROLE_SENDER
  loopSender();
#endif
#ifdef ROLE_RECEIVER
  loopReceiver();
#endif
}
