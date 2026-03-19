// receiver.cpp — ESP-NOW receive callback, POCSAG processing, and receiver setup.
#include "receiver.h"
#include "globals.h"
#include "buzzer.h"
#include "sensor.h"
#include "web_server.h"
#include "display.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <time.h>

// ── POCSAG helpers ────────────────────────────────────────────────────────────

#if RECV_POCSAG

static const char* functionalNameRx(uint8_t f) {
  switch (f) {
    case 0: return "NUMERIC";
    case 1: return "ALERT1";
    case 2: return "ALERT2";
    case 3: return "ALPHA";
    default: return "?";
  }
}

// Parse time beacon from RIC 224.
// Message format: "YYYYMMDDHHMMSS" + "YYMMDDHHmmSS"
// Calls settimeofday() to set the system clock.
static void applyPocsagTime(const char* msg) {
  size_t len = strlen(msg);
  if (len < 26) {
    LOG("[TIME] RIC 224 message too short (%u chars), expected >=26\n", len);
    return;
  }
  const char* d = msg + 14;  // skip the "YYYYMMDDHHMMSS" format label

  char tmp[3] = {};
  struct tm t = {};
  memcpy(tmp, d +  0, 2); t.tm_year = 100 + atoi(tmp); // YY → years since 1900
  memcpy(tmp, d +  2, 2); t.tm_mon  = atoi(tmp) - 1;   // 1-based → 0-based
  memcpy(tmp, d +  4, 2); t.tm_mday = atoi(tmp);
  memcpy(tmp, d +  6, 2); t.tm_hour = atoi(tmp);
  memcpy(tmp, d +  8, 2); t.tm_min  = atoi(tmp);
  memcpy(tmp, d + 10, 2); t.tm_sec  = atoi(tmp);

  time_t epoch = mktime(&t);
  struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
  settimeofday(&tv, nullptr);
  timeSynced   = true;
  pocsagSynced = true;

  if (rtcAvailable)
    ds1307Write(t);

  LOG("[TIME] Set from POCSAG RIC %lu: %04d-%02d-%02d %02d:%02d:%02d%s\n",
    (unsigned long)timePocRic,
    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
    t.tm_hour, t.tm_min, t.tm_sec,
    rtcAvailable ? " [RTC updated]" : "");
}

// Runs on Core 1 (loop()) after being dequeued from pocsagRxQueue.
void processPocsagPacket(const EspNowPocsagPacket& pkt) {
  if (!recvPocsagEnabled) return;

  if (pkt.ric == timePocRic)
    applyPocsagTime(pkt.message);

  bool excluded = false;
  for (int i = 0; i < excludedRicsCount; i++)
    if (pkt.ric == excludedRics[i]) { excluded = true; break; }

  if (!excluded) {
    if (screensaverActive) resetScreensaverIdle();  // restores brightness + clears GIF state
    strncpy(pocsagMsg, pkt.message, POCSAG_MSG_MAX_LEN);
    pocsagMsg[POCSAG_MSG_MAX_LEN] = '\0';
    if (pkt.ric == callsignRic) {
      int len = strlen(pocsagMsg);
      while (len > 0 && pocsagMsg[len - 1] >= '0' && pocsagMsg[len - 1] <= '9')
        pocsagMsg[--len] = '\0';
    }
    pocsagMsgLen    = strlen(pocsagMsg);
    pocsagMsgActive = (pocsagMsgLen > 0);
    if (pocsagMsgActive) buzzerBeep();
    pocsagIsScrolling = (POCSAG_ICON_RESERVED_PX + pocsagMsgLen * 4 - 1 > MATRIX_WIDTH);
    if (pocsagIsScrolling) {
      pocsagScrollX    = MATRIX_WIDTH;
      pocsagScrollPass = 0;
      pocsagScrollLast = millis();
    } else {
      pocsagStaticUntil    = millis() + POCSAG_STATIC_MS;
      pocsagStaticLastDraw = 0;
    }
  }
}

#endif  // RECV_POCSAG

// ── ESP-NOW v2 processor ──────────────────────────────────────────────────────

#if RECV_ESPNOW2

// TODO: define EspNowV2Packet struct (must match sender side)
// TODO: implement time sync from v2 time beacon
// TODO: implement display/message handling for v2 packet types

static void processEspNowV2Packet(const uint8_t* data, int len) {
  // placeholder — v2 protocol not yet implemented
  LOG("[RX-V2] Received %d bytes (not yet handled)\n", len);
}

#endif  // RECV_ESPNOW2

// ── Receive callback (Core 0 WiFi stack) ──────────────────────────────────────

void onReceive(const esp_now_recv_info_t* info, const uint8_t* inData, int inLen) {
  if (inLen < 1) return;
  uint8_t type = inData[0];

  // ── DMR packet ──────────────────────────────────────────────────────────────
#if RECV_DMR
  if (type == ESPNOW_TYPE_DMR_NET) {
    EspNowDmrNetPacket pkt = {};
    memcpy(&pkt, inData, (inLen < (int)sizeof(pkt)) ? inLen : sizeof(pkt));

    if (pkt.len < 21 || memcmp(pkt.data, "DMRD", 4) != 0) {
      LOG("[RX-DMR] Bad DMRD payload (len=%d)\n", pkt.len);
      return;
    }

    rxTotalDmr++;
    wsCountDmr++;
    uint32_t srcId   = ((uint32_t)pkt.data[5] << 16) | ((uint32_t)pkt.data[6] << 8) | pkt.data[7];
    uint32_t dstId   = ((uint32_t)pkt.data[8] << 16) | ((uint32_t)pkt.data[9] << 8) | pkt.data[10];
    uint8_t  flags   = pkt.data[15];
    uint8_t  slot    = (flags & 0x80) ? 2 : 1;
    bool     isGroup = (flags & 0x40) != 0;
    uint8_t  seq     = pkt.data[4];

    bool isNewCall = (srcId != callSrc || dstId != callDst || slot != callSlot);
    if (isNewCall) {
      if (callFrames > 0) {
        unsigned long dur = (millis() - callStart) / 1000;
        LOG("[RX-DMR] ── END   src=%-8lu  dst=TG%-6lu  slot=%d  frames=%lu  dur=%lus\n\n",
          callSrc, callDst, callSlot, callFrames, dur);
      }
      callSrc    = srcId;
      callDst    = dstId;
      callSlot   = slot;
      callFrames = 0;
      callStart  = millis();
      LOG("[RX-DMR] ══ NEW   src=%-8lu  dst=%s%-6lu  slot=%d  pkt#%lu\n",
        srcId, isGroup ? "TG" : "", dstId, slot, rxTotalDmr);
    }
    callFrames++;

#if ESPNOW_DEBUG
    char streamHex[9];
    snprintf(streamHex, sizeof(streamHex), "%02X%02X%02X%02X",
      pkt.data[16], pkt.data[17], pkt.data[18], pkt.data[19]);
    LOG("  [#%lu] seq=%3d  stream=%s  frame: %02X %02X %02X %02X %02X %02X %02X %02X\n",
      callFrames, seq, streamHex,
      pkt.data[20], pkt.data[21], pkt.data[22], pkt.data[23],
      pkt.data[24], pkt.data[25], pkt.data[26], pkt.data[27]);
#else
    (void)seq;
#endif
    return;
  }
#endif  // RECV_DMR

  // ── POCSAG packet ────────────────────────────────────────────────────────────
#if RECV_POCSAG
  if (type == ESPNOW_TYPE_POCSAG) {
    EspNowPocsagPacket pkt = {};
    memcpy(&pkt, inData, (inLen < (int)sizeof(pkt)) ? inLen : sizeof(pkt));
    pkt.message[POCSAG_MSG_MAX_LEN] = '\0';

    rxTotalPocsag++;
    wsCountPocsag++;
    wsPocsagLog[wsPocsagHead].ric = pkt.ric;
    strncpy(wsPocsagLog[wsPocsagHead].msg, pkt.message, POCSAG_MSG_MAX_LEN);
    wsPocsagLog[wsPocsagHead].msg[POCSAG_MSG_MAX_LEN] = '\0';
    wsPocsagHead = (wsPocsagHead + 1) % POCSAG_LOG_SIZE;
    if (wsPocsagFill < POCSAG_LOG_SIZE) wsPocsagFill++;
    LOG("[RX-POCSAG #%lu] RIC=%-10lu  enc=%-7s  msg='%s'\n",
      rxTotalPocsag, (unsigned long)pkt.ric,
      functionalNameRx(pkt.functional), pkt.message);

    // Immediate MQTT push
    mqttNotifyPocsag();
    // Hand off display state update to Core 1 via queue
    xQueueSendFromISR(pocsagRxQueue, &pkt, nullptr);
    return;
  }
#endif  // RECV_POCSAG

#if RECV_ESPNOW2
  // TODO: add ESPNOW_TYPE_V2 once packet type byte is defined
  processEspNowV2Packet(inData, inLen);
  return;
#endif

  LOG("[RX] Unknown type 0x%02X (%d bytes)\n", type, inLen);
}

// ── WiFi + ESP-NOW setup ──────────────────────────────────────────────────────

static void setupReceiverNetwork() {
  WiFi.mode(WIFI_STA);
  if (strlen(WIFI_SSID) > 0) {
    WiFi.setHostname(mdnsName);  // must be set before begin() for DHCP + mDNS
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    LOG("[WiFi] Connecting to %s ", WIFI_SSID);
    unsigned long t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 8000) {
      delay(250); LOG(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      WiFi.setSleep(false);  // prevent WiFi power-save pauses from glitching RMT/WS2812B
      LOG("\n[WiFi] Connected: %s  channel: %d\n",
        WiFi.localIP().toString().c_str(), WiFi.channel());
      setupOTA();            // ArduinoOTA + WebServer (in web_server.cpp)
    } else {
      LOG("\n[WiFi] Not connected\n");
      WiFi.disconnect();
    }
  }
}

void setupReceiver() {
  LOG("[ROLE] RECEIVER — modes:"
#if RECV_DMR
  " DMR"
#endif
#if RECV_POCSAG
  " POCSAG"
#endif
#if RECV_ESPNOW2
  " ESPNOW2"
#endif
  "\n");

#if RECV_DMR && ESPNOW_DEBUG
  LOG("[MODE] DMR debug: ON\n");
#elif RECV_DMR
  LOG("[MODE] DMR debug: OFF (set ESPNOW_DEBUG true for full frames)\n");
#endif

  setupReceiverNetwork();

  if (esp_now_init() != ESP_OK) {
    LOG("[ESP-NOW] Init FAILED — halting.\n");
    while (true) delay(1000);
  }

  uint8_t macBytes[6];
  esp_wifi_get_mac(WIFI_IF_STA, macBytes);
  LOG("[INFO] My MAC : %02X:%02X:%02X:%02X:%02X:%02X\n",
    macBytes[0], macBytes[1], macBytes[2],
    macBytes[3], macBytes[4], macBytes[5]);

  esp_now_register_recv_cb(onReceive);
  LOG("[RECEIVER] Listening — clock will sync on first RIC %lu beacon\n",
    (unsigned long)timePocRic);
}
