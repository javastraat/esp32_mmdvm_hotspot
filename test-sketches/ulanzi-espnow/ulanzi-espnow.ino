/*
 * ESP-NOW Gateway Test Monitor + Clock Display
 *
 * ROLE_SENDER:   sends fake DMRD/POCSAG packets; syncs clock via NTP (WiFi).
 * ROLE_RECEIVER: tries to join WiFi (same router as sender = same channel),
 *                falls back to SoftAP if unavailable. ArduinoOTA always active.
 *                Syncs clock from POCSAG RIC 224 time-beacon
 *                (format "YYYYMMDDHHMMSS<YYMMDDHHmmSS>").
 *
 * Both roles display the current time on the Ulanzi 32×8 LED matrix.
 *
 * Configure role, modes, and intervals in config.h.
 */

#include "config.h"
#include <FastLED.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <time.h>

// ============================================================
// Sanity checks
// ============================================================
#if defined(ROLE_SENDER) && defined(ROLE_RECEIVER)
  #error "Define ROLE_SENDER *or* ROLE_RECEIVER in config.h, not both."
#endif
#if !defined(ROLE_SENDER) && !defined(ROLE_RECEIVER)
  #error "Define ROLE_SENDER or ROLE_RECEIVER in config.h."
#endif
#if TEST_DMR == false && TEST_POCSAG == false
  #error "Enable at least one of TEST_DMR or TEST_POCSAG in config.h."
#endif

// ============================================================
// Packet definitions — MUST match system/system_espnow.h exactly
// ============================================================
#define ESPNOW_TYPE_DMR_NET  0x10
#define ESPNOW_TYPE_POCSAG   0x11

#define POCSAG_MSG_MAX_LEN   80
#define FUNCTIONAL_NUMERIC       0
#define FUNCTIONAL_ALPHANUMERIC  3

struct __attribute__((packed)) EspNowDmrNetPacket {
  uint8_t type;
  uint8_t len;
  uint8_t data[60];
};

struct __attribute__((packed)) EspNowPocsagPacket {
  uint8_t  type;
  uint32_t ric;
  uint8_t  functional;
  char     message[POCSAG_MSG_MAX_LEN + 1];
};

// ============================================================
// LED matrix — 32×8 WS2812B serpentine
// ============================================================
#define NUM_LEDS      256
#define MATRIX_WIDTH   32
#define MATRIX_HEIGHT   8

CRGB leds[NUM_LEDS];

// 3×5 pixel font for digits 0–9
static const uint8_t FONT_DIGITS[10][5] = {
  {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
  {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
  {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
  {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
  {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
  {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
  {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
  {0b111, 0b001, 0b010, 0b010, 0b010}, // 7
  {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
  {0b111, 0b101, 0b111, 0b001, 0b111}  // 9
};

static void setLED(int x, int y, CRGB color) {
  if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) return;
  int idx = (y % 2 == 0) ? y * MATRIX_WIDTH + x : (y + 1) * MATRIX_WIDTH - 1 - x;
  leds[idx] = color;
}

static void drawDigit(int x, int y, int d, CRGB color) {
  for (int row = 0; row < 5; row++)
    for (int col = 0; col < 3; col++)
      if (FONT_DIGITS[d][row] & (1 << (2 - col)))
        setLED(x + col, y + row, color);
}

// Draw HH:MM:SS starting at row 1, fits in 27 columns of the 32-wide matrix
static void drawTime(int h, int m, int s, CRGB color) {
  drawDigit( 2, 1, h / 10, color);
  drawDigit( 6, 1, h % 10, color);
  setLED(10, 2, color); setLED(10, 4, color);  // colon
  drawDigit(12, 1, m / 10, color);
  drawDigit(16, 1, m % 10, color);
  setLED(20, 2, color); setLED(20, 4, color);  // colon
  drawDigit(22, 1, s / 10, color);
  drawDigit(26, 1, s % 10, color);
}

// Update display once per second — call from both role loops
static bool timeSynced = false;

static void loopDisplay() {
  if (!timeSynced) {
    // Scanner animation while waiting for first time beacon
    static unsigned long lastScan = 0;
    if (millis() - lastScan < 40) return;
    lastScan = millis();

    static int scanPos = 0;
    static int scanDir = 1;

    FastLED.clear();
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      int dist = abs(x - scanPos);
      uint8_t bright = (dist == 0) ? 200 : (dist == 1) ? 80 : (dist == 2) ? 30 : (dist == 3) ? 10 : 0;
      if (bright > 0) {
        CRGB col(0, bright / 4, bright);  // blue-ish
        for (int y = 0; y < MATRIX_HEIGHT; y++) setLED(x, y, col);
      }
    }
    FastLED.show();

    scanPos += scanDir;
    if (scanPos >= MATRIX_WIDTH - 1 || scanPos <= 0) scanDir = -scanDir;
    return;
  }

  // Clock — update once per second
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 1000) return;
  lastUpdate = millis();

  struct tm t;
  if (!getLocalTime(&t)) return;

  FastLED.clear();
  drawTime(t.tm_hour, t.tm_min, t.tm_sec, LED_COLOR_TIME);
  FastLED.show();
}


// ============================================================
// SENDER
// ============================================================
#ifdef ROLE_SENDER

static uint8_t peerMac[] = RECEIVER_MAC;
static bool    peerRegistered = false;
static uint8_t seqCounter = 0;

// ── DMR helpers ─────────────────────────────────────────────
#if TEST_DMR

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
  buf[11] = buf[12] = buf[13] = buf[14] = 0x00;
  buf[15] = (slot == 2 ? 0x80 : 0x00) | (group ? 0x40 : 0x00);
  buf[16] = 0xAB; buf[17] = 0xCD; buf[18] = buf[19] = seq;
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

static void loopDmrSender()
{
  static unsigned long lastSend = 0;
  static uint8_t cycle = 0;
  if (millis() - lastSend < DMR_SEND_INTERVAL_MS) return;
  lastSend = millis();

  uint32_t src  = (cycle % 2 == 0) ? 2620123 : 2620456;
  uint32_t dst  = 204;
  uint8_t  slot = (cycle % 2) + 1;

  Serial.printf("\n[TX-DMR] src=%-8lu  dst=TG%-6lu  slot=%d  seq=%d\n",
    src, dst, slot, seqCounter + 1);

  sendDmrdPkt(src, dst, slot, true);
  cycle++;
}

#endif  // TEST_DMR

// ── POCSAG helpers ───────────────────────────────────────────
#if TEST_POCSAG

static void sendPocsagPkt(uint32_t ric, uint8_t functional, const char* msg)
{
  EspNowPocsagPacket pkt = {};
  pkt.type       = ESPNOW_TYPE_POCSAG;
  pkt.ric        = ric;
  pkt.functional = functional;
  strncpy(pkt.message, msg, POCSAG_MSG_MAX_LEN);
  pkt.message[POCSAG_MSG_MAX_LEN] = '\0';

  esp_now_send(peerMac, (uint8_t*)&pkt, sizeof(pkt));
}

static const char* functionalName(uint8_t f) {
  switch (f) {
    case FUNCTIONAL_NUMERIC:      return "NUMERIC";
    case FUNCTIONAL_ALPHANUMERIC: return "ALPHA";
    case 1: return "ALERT1";
    case 2: return "ALERT2";
    default: return "?";
  }
}

static void loopPocsagSender()
{
  static unsigned long lastSend = 0;
  if (millis() - lastSend < POCSAG_INTERVAL_MS) return;
  lastSend = millis();

  Serial.printf("\n[TX-POCSAG] RIC=%-10lu  enc=%-6s  msg='%s'\n",
    (unsigned long)POCSAG_RIC, functionalName(FUNCTIONAL_ALPHANUMERIC), POCSAG_CALLSIGN);

  sendPocsagPkt(POCSAG_RIC, FUNCTIONAL_ALPHANUMERIC, POCSAG_CALLSIGN);
}

#endif  // TEST_POCSAG

// ── Send callback ────────────────────────────────────────────
void onSendResult(const wifi_tx_info_t* info, esp_now_send_status_t status) {
  Serial.printf("  delivery: %s\n", status == ESP_NOW_SEND_SUCCESS ? "ACK" : "NO ACK");
}

// ── Setup / loop ─────────────────────────────────────────────
void setupSender() {
  Serial.print("[ROLE] SENDER — modes:");
#if TEST_DMR
  Serial.print(" DMR");
#endif
#if TEST_POCSAG
  Serial.print(" POCSAG");
#endif
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[WiFi] Connecting to %s ", WIFI_SSID);
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 8000) {
    delay(250); Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] %s\n", WiFi.localIP().toString().c_str());
    // Sync time via NTP
    configTime(NTP_GMT_OFFSET_SEC, NTP_DST_OFFSET_SEC, NTP_SERVER);
    Serial.print("[NTP] Waiting for sync");
    struct tm tmp;
    unsigned long tNtp = millis();
    while (!getLocalTime(&tmp) && millis() - tNtp < 10000) {
      delay(500); Serial.print(".");
    }
    if (getLocalTime(&tmp)) {
      timeSynced = true;
      Serial.printf("\n[NTP] Synced: %04d-%02d-%02d %02d:%02d:%02d\n",
        tmp.tm_year + 1900, tmp.tm_mon + 1, tmp.tm_mday,
        tmp.tm_hour, tmp.tm_min, tmp.tm_sec);
    } else {
      Serial.println("\n[NTP] Sync failed — clock not available");
    }
  } else {
    Serial.println("\n[WiFi] Not connected — ESP-NOW still works, no clock");
  }

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

#if TEST_DMR
  Serial.printf("[DMR]    Sending fake DMRD every %d ms\n", DMR_SEND_INTERVAL_MS);
#endif
#if TEST_POCSAG
  Serial.printf("[POCSAG] Sending '%s' (RIC %lu) every %d ms\n",
    POCSAG_CALLSIGN, (unsigned long)POCSAG_RIC, POCSAG_INTERVAL_MS);
#endif
  Serial.println();
}

void loopSender() {
  if (!peerRegistered) return;
#if TEST_DMR
  loopDmrSender();
#endif
#if TEST_POCSAG
  loopPocsagSender();
#endif
  loopDisplay();
}

#endif  // ROLE_SENDER


// ============================================================
// RECEIVER
// ============================================================
#ifdef ROLE_RECEIVER

// ── DMR state ────────────────────────────────────────────────
#if TEST_DMR
static uint32_t rxTotalDmr   = 0;
static uint32_t callFrames   = 0;
static uint32_t callSrc      = 0;
static uint32_t callDst      = 0;
static uint8_t  callSlot     = 0;
static unsigned long callStart = 0;
#endif

// ── POCSAG state ─────────────────────────────────────────────
#if TEST_POCSAG
static uint32_t rxTotalPocsag = 0;

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
// Example:         YYYYMMDDHHMMSS    260314171800
// Calls settimeofday() to set the system clock.
static void applyPocsagTime(const char* msg) {
  size_t len = strlen(msg);
  if (len < 26) {
    Serial.printf("[TIME] RIC 224 message too short (%u chars), expected >=26\n", len);
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
  timeSynced = true;

  Serial.printf("[TIME] Set from POCSAG RIC %d: %04d-%02d-%02d %02d:%02d:%02d\n",
    TIME_POCSAG_RIC,
    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
    t.tm_hour, t.tm_min, t.tm_sec);
}
#endif  // TEST_POCSAG

// ── Receive callback ─────────────────────────────────────────
void onReceive(const esp_now_recv_info_t* info, const uint8_t* inData, int inLen) {
  if (inLen < 1) return;
  uint8_t type = inData[0];

  // ── DMR packet ──────────────────────────────────────────────
#if TEST_DMR
  if (type == ESPNOW_TYPE_DMR_NET) {
    EspNowDmrNetPacket pkt = {};
    memcpy(&pkt, inData, (inLen < (int)sizeof(pkt)) ? inLen : sizeof(pkt));

    if (pkt.len < 21 || memcmp(pkt.data, "DMRD", 4) != 0) {
      Serial.printf("[RX-DMR] Bad DMRD payload (len=%d)\n", pkt.len);
      return;
    }

    rxTotalDmr++;
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
        Serial.printf("[RX-DMR] ── END   src=%-8lu  dst=TG%-6lu  slot=%d  frames=%lu  dur=%lus\n\n",
          callSrc, callDst, callSlot, callFrames, dur);
      }
      callSrc    = srcId;
      callDst    = dstId;
      callSlot   = slot;
      callFrames = 0;
      callStart  = millis();
      Serial.printf("[RX-DMR] ══ NEW   src=%-8lu  dst=%s%-6lu  slot=%d  pkt#%lu\n",
        srcId, isGroup ? "TG" : "", dstId, slot, rxTotalDmr);
    }
    callFrames++;

#if ESPNOW_DEBUG
    char streamHex[9];
    snprintf(streamHex, sizeof(streamHex), "%02X%02X%02X%02X",
      pkt.data[16], pkt.data[17], pkt.data[18], pkt.data[19]);
    Serial.printf("  [#%lu] seq=%3d  stream=%s  frame: %02X %02X %02X %02X %02X %02X %02X %02X\n",
      callFrames, seq, streamHex,
      pkt.data[20], pkt.data[21], pkt.data[22], pkt.data[23],
      pkt.data[24], pkt.data[25], pkt.data[26], pkt.data[27]);
#else
    (void)seq;
#endif
    return;
  }
#endif  // TEST_DMR

  // ── POCSAG packet ───────────────────────────────────────────
#if TEST_POCSAG
  if (type == ESPNOW_TYPE_POCSAG) {
    EspNowPocsagPacket pkt = {};
    memcpy(&pkt, inData, (inLen < (int)sizeof(pkt)) ? inLen : sizeof(pkt));
    pkt.message[POCSAG_MSG_MAX_LEN] = '\0';

    rxTotalPocsag++;
    Serial.printf("[RX-POCSAG #%lu] RIC=%-10lu  enc=%-7s  msg='%s'\n",
      rxTotalPocsag, (unsigned long)pkt.ric,
      functionalNameRx(pkt.functional), pkt.message);

    // Time beacon
    if (pkt.ric == TIME_POCSAG_RIC) {
      applyPocsagTime(pkt.message);
    }
    return;
  }
#endif  // TEST_POCSAG

  Serial.printf("[RX] Unknown type 0x%02X (%d bytes)\n", type, inLen);
}

// ── WiFi setup ────────────────────────────────────────────────
static void setupReceiverNetwork() {
  WiFi.mode(WIFI_STA);
  if (strlen(WIFI_SSID) > 0) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("[WiFi] Connecting to %s ", WIFI_SSID);
    unsigned long t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 8000) {
      delay(250); Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("\n[WiFi] Connected: %s  channel: %d\n",
        WiFi.localIP().toString().c_str(), WiFi.channel());
    } else {
      Serial.println("\n[WiFi] Not connected");
      WiFi.disconnect();
    }
  }
}

// ── Setup / loop ─────────────────────────────────────────────
void setupReceiver() {
  Serial.print("[ROLE] RECEIVER — modes:");
#if TEST_DMR
  Serial.print(" DMR");
#endif
#if TEST_POCSAG
  Serial.print(" POCSAG");
#endif
  Serial.println();

#if TEST_DMR && ESPNOW_DEBUG
  Serial.println("[MODE] DMR debug: ON");
#elif TEST_DMR
  Serial.println("[MODE] DMR debug: OFF (set ESPNOW_DEBUG true for full frames)");
#endif

  setupReceiverNetwork();

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] Init FAILED — halting.");
    while (true) delay(1000);
  }

  uint8_t macBytes[6];
  esp_wifi_get_mac(WIFI_IF_STA, macBytes);
  Serial.println("[INFO] My MAC (use as RECEIVER_MAC in sender config.h):");
  Serial.printf("       %02X:%02X:%02X:%02X:%02X:%02X\n",
    macBytes[0], macBytes[1], macBytes[2],
    macBytes[3], macBytes[4], macBytes[5]);

  esp_now_register_recv_cb(onReceive);
  Serial.printf("[RECEIVER] Listening — clock will sync on first RIC %d beacon\n\n",
    TIME_POCSAG_RIC);
}

void loopReceiver() {
#if ESPNOW_DEBUG
  static unsigned long lastHb = 0;
  if (millis() - lastHb >= 5000) {
    lastHb = millis();
    Serial.printf("[RX] alive %lus | DMR:%lu POCSAG:%lu\n",
      millis() / 1000,
#if TEST_DMR
      rxTotalDmr,
#else
      0UL,
#endif
#if TEST_POCSAG
      rxTotalPocsag
#else
      0UL
#endif
    );
  }
#endif

#if TEST_DMR && !ESPNOW_DEBUG
  static unsigned long lastPrint = 0;
  if (callFrames > 0 && millis() - lastPrint >= 5000) {
    lastPrint = millis();
    unsigned long dur = (millis() - callStart) / 1000;
    Serial.printf("[RX-DMR] ... src=%-8lu  frames=%lu  dur=%lus\n", callSrc, callFrames, dur);
  }
#endif

  loopDisplay();
}

#endif  // ROLE_RECEIVER


// ============================================================
// Arduino entry points
// ============================================================
void setup() {
  pinMode(15, INPUT_PULLDOWN); // stops high-pitch noise
  pinMode(27, INPUT_PULLUP);
  pinMode(26, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
  Serial.begin(115200);
  delay(3000);
  Serial.println("\n\n=== ESP-NOW Gateway Test Monitor + Clock ===");

  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

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
