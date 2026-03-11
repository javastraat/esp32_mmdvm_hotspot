/*
 * m5dial-espnow.ino  —  ESP-NOW receiver with Mondaine clock display
 *
 * Three screens switched by the rotary encoder:
 *   Screen 0 — CLOCK:  Mondaine-style analog clock (time from RIC 224)
 *   Screen 1 — POCSAG: message (wrapped), RIC
 *   Screen 2 — DMR:    src → dst, slot, TG/PC
 *
 * Time source: POCSAG RIC 224 — format "YYYYMMDDHHMMSS260311130200"
 *              Sent every ~2 minutes by the hotspot. millis() tracks
 *              elapsed time between syncs. RIC 224 is not shown on
 *              the POCSAG screen.
 *
 * Incoming packets auto-switch to the relevant screen.
 *
 * Target: M5Dial (ESP32-S3, round 240×240 display)
 * Board:  M5Stack / arduino-esp32 3.x (IDF 5.x)
 */

#include <Arduino.h>
#include <M5Dial.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <time.h>

// Off-screen buffer for the clock — eliminates flicker on second-hand updates
static LGFX_Sprite clockSprite(&M5Dial.Display);

// ── Config ───────────────────────────────────────────────────────────────────
#define DISPLAY_ROTATION  2   // 0 = normal, 2 = upside-down (180°)
#define TIME_RIC          224 // POCSAG RIC carrying date/time

// ── Packet types — must match system/system_espnow.h ────────────────────────
#define ESPNOW_TYPE_DMR_NET   0x10
#define ESPNOW_TYPE_POCSAG    0x11
#define POCSAG_MSG_MAX_LEN    80

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

// ── State ────────────────────────────────────────────────────────────────────
#define SCREEN_CLOCK   0
#define SCREEN_POCSAG  1
#define SCREEN_DMR     2
#define SCREEN_COUNT   3

static int           currentScreen    = SCREEN_CLOCK;
static long          lastEncoderPos   = 0;
static bool          needsRedraw      = true;
static int           lastDrawnSecond  = -1;
static unsigned long lastPacketMillis = 0;   // for auto-return to clock
#define AUTO_CLOCK_MS  15000                 // return to clock after 15 s

// DMR
static uint32_t lastDmrSrc   = 0;
static uint32_t lastDmrDst   = 0;
static uint8_t  lastDmrSlot  = 0;
static bool     lastDmrGroup = true;

// POCSAG
static uint32_t lastRic = 0;
static char     lastMsg[POCSAG_MSG_MAX_LEN + 1] = "";

// Time — base epoch from RIC 224, advanced with millis()
static time_t         baseEpoch  = 0;
static unsigned long  baseMillis = 0;

// ── Time helpers ──────────────────────────────────────────────────────────────

// Parse "YYYYMMDDHHMMSS260311130200" → set base epoch
// Prefix "YYYYMMDDHHMMSS" (14 chars) is literal; data starts at offset 14.
static void parseTimePacket(const char* msg) {
  if (strlen(msg) < 26) return;
  const char* p = msg + 14;   // points to "YYMMDDHHmmss"
  // validate digits
  for (int i = 0; i < 12; i++) {
    if (p[i] < '0' || p[i] > '9') return;
  }
  struct tm t = {};
  t.tm_year  = 100 + (p[0]-'0')*10 + (p[1]-'0');  // years since 1900
  t.tm_mon   = (p[2]-'0')*10 + (p[3]-'0') - 1;    // 0-based
  t.tm_mday  = (p[4]-'0')*10 + (p[5]-'0');
  t.tm_hour  = (p[6]-'0')*10 + (p[7]-'0');
  t.tm_min   = (p[8]-'0')*10 + (p[9]-'0');
  t.tm_sec   = (p[10]-'0')*10 + (p[11]-'0');
  t.tm_isdst = -1;

  baseEpoch  = mktime(&t);
  baseMillis = millis();
  Serial.printf("[TIME] synced: %04d-%02d-%02d %02d:%02d:%02d\n",
                t.tm_year+1900, t.tm_mon+1, t.tm_mday,
                t.tm_hour, t.tm_min, t.tm_sec);
}

static bool getClockTime(struct tm* t) {
  if (baseEpoch == 0) return false;
  time_t now = baseEpoch + (time_t)((millis() - baseMillis) / 1000UL);
  localtime_r(&now, t);
  return true;
}

// ── Clock drawing ─────────────────────────────────────────────────────────────

// Filled rotated rectangle — used for hands and tick marks.
// angle: degrees clockwise from 12 o'clock.
// front: length in front of pivot, back: length behind pivot.
static void fillHand(float cx, float cy, float angleDeg,
                     float front, float back, float width, uint32_t color) {
  float rad  = (angleDeg - 90.0f) * DEG_TO_RAD;
  float cosA = cosf(rad), sinA = sinf(rad);
  float px   = -sinA, py = cosA;      // perpendicular unit vector
  float hw   = width / 2.0f;

  float x0 = cx - back*cosA + hw*px,  y0 = cy - back*sinA + hw*py;
  float x1 = cx - back*cosA - hw*px,  y1 = cy - back*sinA - hw*py;
  float x2 = cx + front*cosA + hw*px, y2 = cy + front*sinA + hw*py;
  float x3 = cx + front*cosA - hw*px, y3 = cy + front*sinA - hw*py;

  clockSprite.fillTriangle(x0, y0, x1, y1, x2, y2, color);
  clockSprite.fillTriangle(x1, y1, x2, y2, x3, y3, color);
}

static void drawClockFace() {
  const int CX = 120, CY = 120;
  clockSprite.fillCircle(CX, CY, 109, TFT_WHITE);

  // 60 tick marks — hour ticks are wider and longer
  for (int i = 0; i < 60; i++) {
    bool  isHour = (i % 5 == 0);
    float angle  = i * 6.0f;
    float outerR = 106.0f;
    float len    = isHour ? 16.0f :  6.0f;
    float width  = isHour ?  5.5f :  2.0f;
    float midR   = outerR - len / 2.0f;
    float rad    = (angle - 90.0f) * DEG_TO_RAD;
    fillHand(CX + midR*cosf(rad), CY + midR*sinf(rad),
             angle, len/2, len/2, width, TFT_BLACK);
  }
}

static void drawClockHands(int h, int m, int s) {
  const int CX = 120, CY = 120;

  float hourAngle   = (h % 12) * 30.0f + m * 0.5f;
  float minuteAngle = m * 6.0f + s * 0.1f;
  float secondAngle = s * 6.0f;

  // Hour hand — thick, short
  fillHand(CX, CY, hourAngle,   58, 14, 9, TFT_BLACK);
  // Minute hand — thinner, longer
  fillHand(CX, CY, minuteAngle, 80, 14, 7, TFT_BLACK);
  // Second hand — thin red stick
  fillHand(CX, CY, secondAngle, 88, 24, 2, TFT_RED);
  // Red ball on second hand (~70px from center)
  float sRad = (secondAngle - 90.0f) * DEG_TO_RAD;
  clockSprite.fillCircle(CX + 70*cosf(sRad), CY + 70*sinf(sRad), 7, TFT_RED);
  // Center cap
  clockSprite.fillCircle(CX, CY, 5, TFT_RED);
}

// ── Page indicator dots ───────────────────────────────────────────────────────
static void drawPageDots(bool whiteOnBlack = true) {
  int cx = 120, y = 218;
  for (int i = 0; i < SCREEN_COUNT; i++) {
    int x = cx + (i - 1) * 12;   // 3 dots centered: -12, 0, +12
    if (i == currentScreen)
      M5Dial.Display.fillCircle(x, y, 4, whiteOnBlack ? TFT_WHITE : TFT_BLACK);
    else
      M5Dial.Display.drawCircle(x, y, 4, TFT_DARKGREY);
  }
}

// ── Screen draw functions ─────────────────────────────────────────────────────
static void drawClockScreen() {
  // Draw entirely into sprite — push once to avoid flicker
  clockSprite.fillScreen(TFT_BLACK);

  struct tm t;
  bool hasTime = getClockTime(&t);

  if (hasTime) {
    drawClockFace();

    // Date below center — inside the white clock face area
    char dateBuf[16];
    strftime(dateBuf, sizeof(dateBuf), "%a %d %b", &t);
    clockSprite.setTextDatum(TC_DATUM);
    clockSprite.setTextColor(TFT_DARKGREY, TFT_WHITE);
    clockSprite.setTextFont(1);
    clockSprite.drawString(dateBuf, 120, 150);

    drawClockHands(t.tm_hour, t.tm_min, t.tm_sec);
    lastDrawnSecond = t.tm_sec;
  } else {
    clockSprite.setTextDatum(MC_DATUM);
    clockSprite.setTextColor(TFT_DARKGREY, TFT_BLACK);
    clockSprite.setTextFont(2);
    clockSprite.drawString("waiting for", 120, 105);
    clockSprite.drawString("time sync...", 120, 125);
  }

  // Page dots — drawn into sprite
  uint32_t dotColor = hasTime ? TFT_BLACK : TFT_WHITE;
  for (int i = 0; i < SCREEN_COUNT; i++) {
    int x = 120 + (i - 1) * 12;
    if (i == currentScreen)
      clockSprite.fillCircle(x, 218, 4, dotColor);
    else
      clockSprite.drawCircle(x, 218, 4, TFT_DARKGREY);
  }

  clockSprite.pushSprite(0, 0);   // single blit — no flicker
}

static void drawDmrScreen() {
  M5Dial.Display.fillScreen(TFT_BLACK);
  M5Dial.Display.setTextDatum(TC_DATUM);

  M5Dial.Display.setTextColor(TFT_GREEN, TFT_BLACK);
  M5Dial.Display.setTextFont(2);
  M5Dial.Display.drawString("DMR", 120, 18);
  M5Dial.Display.drawFastHLine(50, 38, 140, TFT_DARKGREY);

  if (lastDmrSrc > 0) {
    M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Dial.Display.setTextFont(4);
    M5Dial.Display.drawString(String(lastDmrSrc), 120, 52);

    M5Dial.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5Dial.Display.setTextFont(2);
    M5Dial.Display.drawString("to", 120, 94);

    M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Dial.Display.setTextFont(4);
    M5Dial.Display.drawString(String(lastDmrDst), 120, 112);

    char info[16];
    snprintf(info, sizeof(info), "TS%u  %s", lastDmrSlot, lastDmrGroup ? "TG" : "PC");
    M5Dial.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5Dial.Display.setTextFont(2);
    M5Dial.Display.drawString(info, 120, 162);
  } else {
    M5Dial.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5Dial.Display.setTextFont(2);
    M5Dial.Display.drawString("no data", 120, 110);
  }

  drawPageDots();
}

static void drawPocsagScreen() {
  M5Dial.Display.fillScreen(TFT_BLACK);
  M5Dial.Display.setTextDatum(TC_DATUM);

  M5Dial.Display.setTextColor(TFT_ORANGE, TFT_BLACK);
  M5Dial.Display.setTextFont(2);
  M5Dial.Display.drawString("POCSAG", 120, 18);
  M5Dial.Display.drawFastHLine(50, 38, 140, TFT_DARKGREY);

  if (lastRic > 0) {
    M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Dial.Display.setTextFont(2);
    String msg = String(lastMsg);
    int y = 52;
    for (int offset = 0; offset < (int)msg.length() && offset < 48; offset += 16) {
      M5Dial.Display.drawString(msg.substring(offset, offset + 16), 120, y);
      y += 22;
    }
    char ricLine[20];
    snprintf(ricLine, sizeof(ricLine), "RIC %u", lastRic);
    M5Dial.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5Dial.Display.drawString(ricLine, 120, 168);
  } else {
    M5Dial.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5Dial.Display.setTextFont(2);
    M5Dial.Display.drawString("no data", 120, 110);
  }

  drawPageDots();
}

static void redraw() {
  switch (currentScreen) {
    case SCREEN_POCSAG: drawPocsagScreen(); break;
    case SCREEN_CLOCK:  drawClockScreen();  break;
    case SCREEN_DMR:    drawDmrScreen();    break;
  }
}

// ── ESP-NOW receive callback ──────────────────────────────────────────────────
static void onReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (len < 2) return;

  const uint8_t* m = info->src_addr;
  char mac[18];
  snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
           m[0], m[1], m[2], m[3], m[4], m[5]);

  if (data[0] == ESPNOW_TYPE_DMR_NET) {
    EspNowDmrNetPacket pkt = {};
    memcpy(&pkt, data, min(len, (int)sizeof(pkt)));
    if (pkt.len < 16) return;
    const uint8_t* d = pkt.data;
    if (d[0]!='D'||d[1]!='M'||d[2]!='R'||d[3]!='D') return;

    lastDmrSrc   = ((uint32_t)d[5]<<16)|((uint32_t)d[6]<<8)|d[7];
    lastDmrDst   = ((uint32_t)d[8]<<16)|((uint32_t)d[9]<<8)|d[10];
    lastDmrSlot  = (d[15] & 0x80) ? 2 : 1;
    lastDmrGroup = (d[15] & 0x40) == 0;
    currentScreen    = SCREEN_DMR;
    lastPacketMillis = millis();
    needsRedraw      = true;
    Serial.printf("[DMR] from %s  src=%u dst=%u TS%u %s\n",
                  mac, lastDmrSrc, lastDmrDst, lastDmrSlot, lastDmrGroup ? "TG" : "PC");
  }
  else if (data[0] == ESPNOW_TYPE_POCSAG) {
    EspNowPocsagPacket pkt = {};
    memcpy(&pkt, data, min(len, (int)sizeof(pkt)));
    pkt.message[POCSAG_MSG_MAX_LEN] = '\0';

    Serial.printf("[POCSAG] from %s  RIC=%u func=%u msg=\"%s\"\n",
                  mac, pkt.ric, pkt.functional, pkt.message);

    if (pkt.ric == TIME_RIC) {
      // Time sync packet — parse and update clock
      parseTimePacket(pkt.message);
      if (currentScreen == SCREEN_CLOCK) needsRedraw = true;
    }

    lastRic = pkt.ric;
    strncpy(lastMsg, pkt.message, POCSAG_MSG_MAX_LEN);
    lastMsg[POCSAG_MSG_MAX_LEN] = '\0';
    currentScreen    = SCREEN_POCSAG;
    lastPacketMillis = millis();
    needsRedraw      = true;
  }
}

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
  auto cfg = M5.config();
  M5Dial.begin(cfg, true, false);   // enable encoder, disable IR

  clockSprite.setColorDepth(16);
  clockSprite.createSprite(240, 240);

  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== m5dial-espnow ===");

  M5Dial.Display.setRotation(DISPLAY_ROTATION);

  // Init WiFi in STA mode for ESP-NOW (no AP connection needed)
  WiFi.mode(WIFI_STA);
  delay(100);

  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init FAILED");
    M5Dial.Display.fillScreen(TFT_BLACK);
    M5Dial.Display.setTextDatum(MC_DATUM);
    M5Dial.Display.setTextColor(TFT_RED, TFT_BLACK);
    M5Dial.Display.setTextFont(2);
    M5Dial.Display.drawString("ESP-NOW FAILED", 120, 120);
    return;
  }
  esp_now_register_recv_cb(onReceive);
  Serial.println("ESP-NOW ready — waiting for time sync on RIC 224...");

  lastEncoderPos = M5Dial.Encoder.read();
  needsRedraw = true;
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {
  M5Dial.update();

  // Encoder turn → cycle screens
  long pos = M5Dial.Encoder.read();
  if (pos != lastEncoderPos) {
    int dir = (pos > lastEncoderPos) ? 1 : -1;
    lastEncoderPos = pos;
    currentScreen  = (currentScreen + dir + SCREEN_COUNT) % SCREEN_COUNT;
    lastDrawnSecond = -1;
    needsRedraw     = true;
  }

  // Auto-return to clock after 15 s of no new packets
  if (currentScreen != SCREEN_CLOCK && lastPacketMillis > 0 &&
      millis() - lastPacketMillis >= AUTO_CLOCK_MS) {
    lastPacketMillis = 0;
    currentScreen    = SCREEN_CLOCK;
    lastDrawnSecond  = -1;
    needsRedraw      = true;
  }

  // Clock screen: redraw every second
  if (currentScreen == SCREEN_CLOCK && !needsRedraw) {
    struct tm t;
    if (getClockTime(&t) && t.tm_sec != lastDrawnSecond)
      needsRedraw = true;
  }

  if (needsRedraw) {
    needsRedraw = false;
    redraw();
  }

  delay(50);
}
