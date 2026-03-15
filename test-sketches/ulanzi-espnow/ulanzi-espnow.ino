/*
 * ESP-NOW Receiver + Clock Display (Ulanzi TC001)
 *
 * Receives DMRD Homebrew + POCSAG packets from the MMDVM hotspot via ESP-NOW.
 * Tries to join WiFi (same router as hotspot = same channel).
 * Syncs clock from POCSAG RIC 224 time-beacon ("YYYYMMDDHHMMSS<YYMMDDHHmmSS>").
 * ArduinoOTA + web status page always active when WiFi is up.
 *
 * Configure modes and display settings in config.h.
 */

#include "config.h"
#include <FastLED.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <time.h>
#include "web/main.h"

// ============================================================
// Sanity checks
// ============================================================
#if RECV_DMR == false && RECV_POCSAG == false
  #error "Enable at least one of RECV_DMR or RECV_POCSAG in config.h."
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

// Draw HH:MM:SS centered in the 32×8 matrix (27px content, 5px digit height)
static void drawTime(int h, int m, int s, CRGB color) {
  const int xo = (MATRIX_WIDTH  - 27 + 1) / 2;  // = 3  (3px left, 2px right)
  const int yo = (MATRIX_HEIGHT -  5)     / 2;  // = 1  (1px top, 2px bottom)
  drawDigit(xo +  0, yo, h / 10, color);
  drawDigit(xo +  4, yo, h % 10, color);
  setLED(xo +  8, yo + 1, color); setLED(xo +  8, yo + 3, color);  // colon
  drawDigit(xo + 10, yo, m / 10, color);
  drawDigit(xo + 14, yo, m % 10, color);
  setLED(xo + 18, yo + 1, color); setLED(xo + 18, yo + 3, color);  // colon
  drawDigit(xo + 20, yo, s / 10, color);
  drawDigit(xo + 24, yo, s % 10, color);
}

// 3×5 bitmaps for the letters in "UPDATE"
static const uint8_t FONT_UPDATE[6][5] = {
  {0b101, 0b101, 0b101, 0b101, 0b111}, // U
  {0b111, 0b101, 0b111, 0b100, 0b100}, // P
  {0b110, 0b101, 0b101, 0b101, 0b110}, // D
  {0b111, 0b101, 0b111, 0b101, 0b101}, // A
  {0b111, 0b010, 0b010, 0b010, 0b010}, // T
  {0b111, 0b100, 0b111, 0b100, 0b111}, // E
};

// Additional 3×5 bitmaps for "DONE" and "ERR"
static const uint8_t FONT_DONE[4][5] = {
  {0b110, 0b101, 0b101, 0b101, 0b110}, // D
  {0b111, 0b101, 0b101, 0b101, 0b111}, // O
  {0b101, 0b111, 0b101, 0b101, 0b101}, // N
  {0b111, 0b100, 0b111, 0b100, 0b111}, // E
};

static const uint8_t FONT_ERROR[5][5] = {
  {0b111, 0b100, 0b111, 0b100, 0b111}, // E
  {0b111, 0b101, 0b111, 0b110, 0b101}, // R
  {0b111, 0b101, 0b111, 0b110, 0b101}, // R
  {0b111, 0b101, 0b101, 0b101, 0b111}, // O
  {0b111, 0b101, 0b111, 0b110, 0b101}, // R
};

// Internal: render glyphs only — no clear/show
static void _drawGlyphs(const uint8_t glyphs[][5], int count, CRGB color, int xo, int yo) {
  for (int i = 0; i < count; i++)
    for (int row = 0; row < 5; row++)
      for (int col = 0; col < 3; col++)
        if (glyphs[i][row] & (1 << (2 - col)))
          setLED(xo + i * 4 + col, yo + row, color);
}

static void drawStatusWord(const uint8_t glyphs[][5], int count, CRGB color) {
  FastLED.clear();
  int width = count * 4 - 1;
  _drawGlyphs(glyphs, count, color, (MATRIX_WIDTH - width + 1) / 2, (MATRIX_HEIGHT - 5) / 2);
  FastLED.show();
}

// drawUpdate: called once on OTA start — sets text + full dim track on row 7
static void drawUpdate() {
  FastLED.clear();
  _drawGlyphs(FONT_UPDATE, 6, LED_COLOR_TIME,
              (MATRIX_WIDTH - 23 + 1) / 2, (MATRIX_HEIGHT - 5) / 2);
  for (int x = 0; x < MATRIX_WIDTH; x++)
    setLED(x, 7, CRGB(0, 25, 25));  // dim track, row 7 only
  FastLED.show();
}

// drawProgress: redraws the full bar row on every call — corrects any corruption
static void drawProgress(int barW) {
  for (int x = 0; x < MATRIX_WIDTH; x++)
    setLED(x, 7, (x < barW) ? CRGB::Cyan : CRGB(0, 25, 25));
  FastLED.show();
}
static void drawDone()   { drawStatusWord(FONT_DONE,   4, CRGB::Green);    }
static void drawError()  { drawStatusWord(FONT_ERROR,  5, CRGB::Red);      }

// 3×5 font for A–Z
static const uint8_t FONT_ALPHA[26][5] = {
  {0b010,0b101,0b111,0b101,0b101}, // A
  {0b110,0b101,0b110,0b101,0b110}, // B
  {0b011,0b100,0b100,0b100,0b011}, // C
  {0b110,0b101,0b101,0b101,0b110}, // D
  {0b111,0b100,0b110,0b100,0b111}, // E
  {0b111,0b100,0b110,0b100,0b100}, // F
  {0b011,0b100,0b101,0b101,0b011}, // G
  {0b101,0b101,0b111,0b101,0b101}, // H
  {0b111,0b010,0b010,0b010,0b111}, // I
  {0b001,0b001,0b001,0b101,0b010}, // J
  {0b101,0b101,0b110,0b101,0b101}, // K
  {0b100,0b100,0b100,0b100,0b111}, // L
  {0b101,0b111,0b111,0b101,0b101}, // M
  {0b101,0b111,0b101,0b101,0b101}, // N
  {0b010,0b101,0b101,0b101,0b010}, // O
  {0b110,0b101,0b110,0b100,0b100}, // P
  {0b010,0b101,0b101,0b011,0b001}, // Q
  {0b110,0b101,0b110,0b101,0b101}, // R
  {0b011,0b100,0b010,0b001,0b110}, // S
  {0b111,0b010,0b010,0b010,0b010}, // T
  {0b101,0b101,0b101,0b101,0b111}, // U
  {0b101,0b101,0b101,0b101,0b010}, // V
  {0b101,0b101,0b111,0b111,0b101}, // W
  {0b101,0b101,0b010,0b101,0b101}, // X
  {0b101,0b101,0b010,0b010,0b010}, // Y
  {0b111,0b001,0b010,0b100,0b111}, // Z
};

// 3×5 bitmaps for punctuation/symbols found in POCSAG weather messages
static const char    SPECIAL_CHARS[]   = "-.:/%=";
static const uint8_t FONT_SPECIAL[][5] = {
  {0b000,0b000,0b111,0b000,0b000}, // -
  {0b000,0b000,0b000,0b000,0b010}, // .
  {0b000,0b010,0b000,0b010,0b000}, // :
  {0b001,0b001,0b010,0b100,0b100}, // /
  {0b101,0b001,0b010,0b100,0b101}, // %
  {0b000,0b111,0b000,0b111,0b000}, // =
};

static void drawChar(int x, int y, char c, CRGB color) {
  if (c >= 'a' && c <= 'z') c -= 32;
  if (c >= '0' && c <= '9') { drawDigit(x, y, c - '0', color); return; }
  if (c >= 'A' && c <= 'Z') {
    const uint8_t* g = FONT_ALPHA[c - 'A'];
    for (int row = 0; row < 5; row++)
      for (int col = 0; col < 3; col++)
        if (g[row] & (1 << (2 - col)))
          setLED(x + col, y + row, color);
    return;
  }
  for (int i = 0; SPECIAL_CHARS[i]; i++) {
    if (SPECIAL_CHARS[i] == c) {
      const uint8_t* g = FONT_SPECIAL[i];
      for (int row = 0; row < 5; row++)
        for (int col = 0; col < 3; col++)
          if (g[row] & (1 << (2 - col)))
            setLED(x + col, y + row, color);
      return;
    }
  }
  // space and other unknowns render as a blank gap (no pixels set)
}

#if RECV_POCSAG
static char  pocsagMsg[POCSAG_MSG_MAX_LEN + 1] = {};
static int   pocsagMsgLen        = 0;
static bool  pocsagMsgActive     = false;
static bool  pocsagIsScrolling   = false;
// scroll-mode state
static int   pocsagScrollX       = 0;
static int   pocsagScrollPass    = 0;
static unsigned long pocsagScrollLast = 0;
// static-mode state
static unsigned long pocsagStaticUntil   = 0;
static unsigned long pocsagStaticLastDraw = 0;  // 0 = force immediate draw
#endif

// ============================================================
// Brightness (LDR auto + manual)
// ============================================================
static bool    autoBrightnessEnabled = true;
static uint8_t currentBrightness     = LED_BRIGHTNESS;

static void loopBrightness() {
  if (!autoBrightnessEnabled) return;
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < LDR_UPDATE_MS) return;
  lastUpdate = millis();

  int ldr = constrain(analogRead(LDR_PIN), LDR_ADC_DARK, LDR_ADC_BRIGHT);
  uint8_t target = (uint8_t)map(ldr, LDR_ADC_DARK, LDR_ADC_BRIGHT, LDR_MIN_BRIGHTNESS, 255);
  // EMA smoothing: blend 1/4 toward target each sample
  currentBrightness = (uint8_t)((currentBrightness * 3 + target + 2) / 4);
  FastLED.setBrightness(currentBrightness);
}

// ============================================================
// Buttons — debounced, fires once per press
// ============================================================
static void loopButtons() {
  static bool          lastState[3]  = {HIGH, HIGH, HIGH};
  static bool          fired[3]      = {false, false, false};
  static unsigned long lastChange[3] = {0, 0, 0};
  static const uint8_t pins[3]       = {BTN_LEFT, BTN_MIDDLE, BTN_RIGHT};

  for (int i = 0; i < 3; i++) {
    bool state = digitalRead(pins[i]);
    if (state != lastState[i]) {
      lastChange[i] = millis();
      lastState[i]  = state;
      fired[i]      = false;
    }
    if (!fired[i] && state == LOW && millis() - lastChange[i] >= BTN_DEBOUNCE_MS) {
      fired[i] = true;
      switch (i) {
        case 0:  // Left — reserved for display mode (step 5)
          Serial.println("[BTN] Left");
          break;
        case 1:  // Middle — toggle auto-brightness
          autoBrightnessEnabled = !autoBrightnessEnabled;
          if (!autoBrightnessEnabled)
            FastLED.setBrightness(currentBrightness);
          Serial.printf("[BTN] Middle — auto brightness: %s\n",
            autoBrightnessEnabled ? "ON" : "OFF");
          break;
        case 2:  // Right — reserved for display mode (step 5)
          Serial.println("[BTN] Right");
          break;
      }
    }
  }
}


// ============================================================
// Web status (updated by role code, served via /api/status)
// ============================================================
static uint32_t  wsCountDmr    = 0;
static uint32_t  wsCountPocsag = 0;
static char      wsLastPocsag[POCSAG_MSG_MAX_LEN + 1] = {};
static WebServer webServer(80);

// Update display once per second — call from both role loops
static bool timeSynced    = false;
static bool otaInProgress = false;  // blocks loopDisplay during OTA flash

static void loopDisplay() {
  if (otaInProgress) return;  // OTA owns the display — don't touch it
  // POCSAG message display — takes priority over both clock and scanner
#if RECV_POCSAG
  if (pocsagMsgActive) {
    const int yo = (MATRIX_HEIGHT - 5) / 2;
    if (!pocsagIsScrolling) {
      // Static: redraw every 500 ms so a clock tick can't erase it
      if (millis() - pocsagStaticLastDraw >= 500) {
        bool first = (pocsagStaticLastDraw == 0);
        pocsagStaticLastDraw = millis();
        int totalW = pocsagMsgLen * 4 - 1;
        int xo = (MATRIX_WIDTH - totalW) / 2;
        FastLED.clear();
        for (int i = 0; i < pocsagMsgLen; i++)
          drawChar(xo + i * 4, yo, pocsagMsg[i], LED_COLOR_POCSAG);
        FastLED.show();
        if (first)
          Serial.printf("[DISP] POCSAG '%s'\n", pocsagMsg);
      }
      if (millis() >= pocsagStaticUntil)
        pocsagMsgActive = false;
    } else {
      // Scroll 3 passes, 50 ms per pixel
      if (millis() - pocsagScrollLast < POCSAG_SCROLL_SPEED_MS) return;
      pocsagScrollLast = millis();
      FastLED.clear();
      for (int i = 0; i < pocsagMsgLen; i++)
        drawChar(pocsagScrollX + i * 4, yo, pocsagMsg[i], LED_COLOR_POCSAG);
      FastLED.show();
      pocsagScrollX--;
      if (pocsagScrollX < -(pocsagMsgLen * 4)) {
        if (++pocsagScrollPass >= POCSAG_SCROLL_PASSES)
          pocsagMsgActive = false;
        else
          pocsagScrollX = MATRIX_WIDTH;
      }
    }
    return;
  }
#endif

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
// OTA (shared by both roles — call only when WiFi is up)
// ============================================================
static bool otaStarted  = false;
static int  otaLastBarW = -1;   // reset each OTA session in onStart

static void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  if (strlen(OTA_PASSWORD) > 0) ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    Serial.println("[OTA] Start");
    otaInProgress = true;
    otaLastBarW   = -1;
    drawUpdate();
  });
  ArduinoOTA.onProgress([](unsigned int current, unsigned int total) {
    int barW = (total > 0) ? (int)((long)MATRIX_WIDTH * current / total) : 0;
    if (barW != otaLastBarW) { otaLastBarW = barW; drawProgress(barW); }
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA] Done — rebooting");
    delay(500);                   // let WiFi/OTA stack finish before touching display
    FastLED.clear(); FastLED.show();
    delay(200);                   // allow the clear to fully latch
    drawDone();
    delay(1500);
    otaInProgress = false;
  });
  ArduinoOTA.onError([](ota_error_t e) {
    Serial.printf("[OTA] Error %u\n", e);
    otaInProgress = false;
    drawError();
    delay(3000);
    timeSynced = false;  // trigger scanner animation until clock resyncs
  });
  ArduinoOTA.begin();
  otaStarted = true;
  Serial.printf("[OTA] Ready — hostname: %s  port: 3232\n", OTA_HOSTNAME);
  setupWebServer();
}

static void setupWebServer() {
  webServer.on("/", HTTP_GET, []() {
    webServer.send_P(200, "text/html", PAGE_MAIN);
  });

  webServer.on("/api/status", HTTP_GET, []() {
    char json[900];
    struct tm t;
    bool hasTm = getLocalTime(&t);
    char timeStr[12] = "--:--:--";
    if (hasTm) snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
                        t.tm_hour, t.tm_min, t.tm_sec);

    // Sanitise last POCSAG: replace " and \ so they don't break JSON
    char safe[POCSAG_MSG_MAX_LEN + 1];
    int si = 0;
    for (int i = 0; wsLastPocsag[i] && si < POCSAG_MSG_MAX_LEN; i++) {
      char c = wsLastPocsag[i];
      if (c == '"' || c == '\\') continue;  // skip unsafe chars
      safe[si++] = c;
    }
    safe[si] = '\0';

    int batRaw = analogRead(BAT_PIN);
    int batMv  = (int)map(constrain(batRaw, BAT_RAW_EMPTY, BAT_RAW_FULL), BAT_RAW_EMPTY, BAT_RAW_FULL, BAT_EMPTY_MV, BAT_FULL_MV);
    int batPct = (int)constrain(map(batRaw, BAT_RAW_EMPTY, BAT_RAW_FULL, 0, 100), 0, 100);

    snprintf(json, sizeof(json),
      "{\"hostname\":\"%s\",\"role\":\"%s\",\"ip\":\"%s\","
      "\"channel\":%d,\"uptime\":%lu,"
      "\"time_synced\":%s,\"time\":\"%s\","
      "\"dmr_count\":%lu,\"pocsag_count\":%lu,"
      "\"last_pocsag\":\"%s\","
      "\"brightness\":%d,\"auto_brightness\":%s,\"ldr_raw\":%d,"
      "\"battery_raw\":%d,\"battery_mv\":%d,\"battery_pct\":%d,"
      "\"mac\":\"%s\",\"ssid\":\"%s\",\"rssi\":%d,\"free_heap\":%u}",
      OTA_HOSTNAME,
      "RECEIVER",
      WiFi.localIP().toString().c_str(),
      WiFi.channel(),
      millis() / 1000,
      timeSynced ? "true" : "false",
      timeStr,
      (unsigned long)wsCountDmr,
      (unsigned long)wsCountPocsag,
      safe,
      currentBrightness,
      autoBrightnessEnabled ? "true" : "false",
      analogRead(LDR_PIN),
      batRaw,
      batMv,
      batPct,
      WiFi.macAddress().c_str(),
      WiFi.SSID().c_str(),
      WiFi.RSSI(),
      ESP.getFreeHeap()
    );
    webServer.send(200, "application/json", json);
  });

  webServer.on("/api/brightness", HTTP_POST, []() {
    String autoArg  = webServer.arg("auto");
    String levelArg = webServer.arg("level");
    if (autoArg.length() > 0)
      autoBrightnessEnabled = (autoArg == "1" || autoArg == "true");
    if (levelArg.length() > 0) {
      int lvl = levelArg.toInt();
      if (lvl >= 1 && lvl <= 255) currentBrightness = (uint8_t)lvl;
    }
    if (!autoBrightnessEnabled)
      FastLED.setBrightness(currentBrightness);
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  webServer.begin();
  Serial.printf("[WEB] Started at http://%s/\n", WiFi.localIP().toString().c_str());
}


// ============================================================
// RECEIVER
// ============================================================

// ── DMR state ────────────────────────────────────────────────
#if RECV_DMR
static uint32_t rxTotalDmr   = 0;
static uint32_t callFrames   = 0;
static uint32_t callSrc      = 0;
static uint32_t callDst      = 0;
static uint8_t  callSlot     = 0;
static unsigned long callStart = 0;
#endif

// ── POCSAG state ─────────────────────────────────────────────
#if RECV_POCSAG
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
#endif  // RECV_POCSAG

// ── Receive callback ─────────────────────────────────────────
void onReceive(const esp_now_recv_info_t* info, const uint8_t* inData, int inLen) {
  if (inLen < 1) return;
  uint8_t type = inData[0];

  // ── DMR packet ──────────────────────────────────────────────
#if RECV_DMR
  if (type == ESPNOW_TYPE_DMR_NET) {
    EspNowDmrNetPacket pkt = {};
    memcpy(&pkt, inData, (inLen < (int)sizeof(pkt)) ? inLen : sizeof(pkt));

    if (pkt.len < 21 || memcmp(pkt.data, "DMRD", 4) != 0) {
      Serial.printf("[RX-DMR] Bad DMRD payload (len=%d)\n", pkt.len);
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
#endif  // RECV_DMR

  // ── POCSAG packet ───────────────────────────────────────────
#if RECV_POCSAG
  if (type == ESPNOW_TYPE_POCSAG) {
    EspNowPocsagPacket pkt = {};
    memcpy(&pkt, inData, (inLen < (int)sizeof(pkt)) ? inLen : sizeof(pkt));
    pkt.message[POCSAG_MSG_MAX_LEN] = '\0';

    rxTotalPocsag++;
    wsCountPocsag++;
    strncpy(wsLastPocsag, pkt.message, POCSAG_MSG_MAX_LEN);
    wsLastPocsag[POCSAG_MSG_MAX_LEN] = '\0';
    Serial.printf("[RX-POCSAG #%lu] RIC=%-10lu  enc=%-7s  msg='%s'\n",
      rxTotalPocsag, (unsigned long)pkt.ric,
      functionalNameRx(pkt.functional), pkt.message);

    // Time beacon — always apply regardless of exclude list
    if (pkt.ric == TIME_POCSAG_RIC)
      applyPocsagTime(pkt.message);

    // Display on LED matrix unless RIC is excluded
    static const uint32_t excludedRics[] = POCSAG_DISPLAY_EXCLUDED_RICS;
    bool excluded = false;
    for (size_t i = 0; i < sizeof(excludedRics) / sizeof(excludedRics[0]); i++)
      if (pkt.ric == excludedRics[i]) { excluded = true; break; }

    if (!excluded) {
      strncpy(pocsagMsg, pkt.message, POCSAG_MSG_MAX_LEN);
      pocsagMsg[POCSAG_MSG_MAX_LEN] = '\0';
      // Strip trailing digits from callsign RIC (maintainer sometimes appends "1" etc.)
      if (pkt.ric == CALLSIGN_RIC) {
        int len = strlen(pocsagMsg);
        while (len > 0 && pocsagMsg[len - 1] >= '0' && pocsagMsg[len - 1] <= '9')
          pocsagMsg[--len] = '\0';
      }
      pocsagMsgLen      = strlen(pocsagMsg);
      pocsagMsgActive   = (pocsagMsgLen > 0);
      // fits on screen (≤8 chars) → static 15 s; otherwise → scroll 3×
      pocsagIsScrolling = (pocsagMsgLen * 4 > MATRIX_WIDTH);
      if (pocsagIsScrolling) {
        pocsagScrollX    = MATRIX_WIDTH;
        pocsagScrollPass = 0;
        pocsagScrollLast = millis();
      } else {
        pocsagStaticUntil    = millis() + POCSAG_STATIC_MS;
        pocsagStaticLastDraw = 0;  // force immediate draw
      }
    }
    return;
  }
#endif  // RECV_POCSAG

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
      WiFi.setSleep(false);  // prevent WiFi power-save pauses from glitching RMT/WS2812B
      Serial.printf("\n[WiFi] Connected: %s  channel: %d\n",
        WiFi.localIP().toString().c_str(), WiFi.channel());
      setupOTA();
    } else {
      Serial.println("\n[WiFi] Not connected");
      WiFi.disconnect();
    }
  }
}

// ── Setup / loop ─────────────────────────────────────────────
void setupReceiver() {
  Serial.print("[ROLE] RECEIVER — modes:");
#if RECV_DMR
  Serial.print(" DMR");
#endif
#if RECV_POCSAG
  Serial.print(" POCSAG");
#endif
  Serial.println();

#if RECV_DMR && ESPNOW_DEBUG
  Serial.println("[MODE] DMR debug: ON");
#elif RECV_DMR
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
  if (otaStarted) ArduinoOTA.handle();
  webServer.handleClient();
#if ESPNOW_DEBUG
  static unsigned long lastHb = 0;
  if (millis() - lastHb >= 5000) {
    lastHb = millis();
    Serial.printf("[RX] alive %lus | DMR:%lu POCSAG:%lu\n",
      millis() / 1000,
#if RECV_DMR
      rxTotalDmr,
#else
      0UL,
#endif
#if RECV_POCSAG
      rxTotalPocsag
#else
      0UL
#endif
    );
  }
#endif

#if RECV_DMR && !ESPNOW_DEBUG
  static unsigned long lastPrint = 0;
  if (callFrames > 0 && millis() - lastPrint >= 5000) {
    lastPrint = millis();
    unsigned long dur = (millis() - callStart) / 1000;
    Serial.printf("[RX-DMR] ... src=%-8lu  frames=%lu  dur=%lus\n", callSrc, callFrames, dur);
  }
#endif

  loopButtons();
  loopBrightness();
  loopDisplay();
}


// ============================================================
// Arduino entry points
// ============================================================
void setup() {
  pinMode(15,         INPUT_PULLDOWN); // buzzer — stops high-pitch noise
  pinMode(BTN_LEFT,   INPUT_PULLUP);
  pinMode(BTN_MIDDLE, INPUT_PULLUP);
  pinMode(BTN_RIGHT,  INPUT_PULLUP);
  pinMode(BAT_PIN, INPUT);     // battery ADC — explicit INPUT per TC001 reference
  Serial.begin(115200);
  delay(3000);
  Serial.println("\n\n=== ESP-NOW Gateway Test Monitor + Clock ===");

  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  setupReceiver();
}

void loop() {
  loopReceiver();
}
