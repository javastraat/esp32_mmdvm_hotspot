/*
 * twatch-espnow.ino  —  ESP-NOW receiver for TTGO T-Watch 2020
 *
 * Three screens, tap the touchscreen to cycle:
 *   Screen 0 — CLOCK:  Mondaine-style analog clock (time from RIC 224)
 *   Screen 1 — POCSAG: message (wrapped), RIC
 *   Screen 2 — DMR:    src → dst, slot, TG/PC
 *
 * Time source: POCSAG RIC 224 — same format as hotspot.
 *              RTC PCF8563 seeds clock on boot; updated on every sync.
 *
 * Board:  TTGO T-Watch 2020 (ESP32, ST7789 240×240, FT6236 touch, PCF8563 RTC)
 * Lib:    TTGO_TWatch_Library  (LilyGoWatch.h)
 * Core:   arduino-esp32 3.x  (IDF 5.x) — ESP-NOW 3.x callback signature
 */

// ── Select your T-Watch 2020 hardware revision ────────────────────────────────
#define LILYGO_WATCH_2020_V1
#include <LilyGoWatch.h>

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
Preferences preferences;
bool wifiEnabled = true;
String wifiSsid = "TechInc";
String wifiPass = "itoldyoualready";
bool apMode = false;
void saveWifiSettings(const String& ssid, const String& pass) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("pass", pass);
  preferences.end();
}

void loadWifiSettings() {
  preferences.begin("wifi", true);
  wifiSsid = preferences.getString("ssid", "TechInc");
  wifiPass = preferences.getString("pass", "itoldyoualready");
  preferences.end();
}
void handleWifiConfig() {
  String message = "";
  if (server.method() == HTTP_POST) {
    wifiSsid = server.arg("ssid");
    wifiPass = server.arg("pass");
    wifiEnabled = server.hasArg("wifiEnabled");
    saveWifiSettings(wifiSsid, wifiPass);
    message = "<div class='card' style='color:green;'>Settings saved. Rebooting...</div>";
    delay(500);
    ESP.restart();
    return;
  }
  String checked = wifiEnabled ? "checked" : "";
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>WiFi Settings</title>"
    "<style>:root{--bg:#181c20;--card:#23272b;--text:#fff;--accent:#35a;--border:#444;}body{background:var(--bg);color:var(--text);font-family:sans-serif;margin:0;}nav{background:var(--card);padding:1em;text-align:center;}nav a{color:var(--accent);margin:0 1em;text-decoration:none;}nav a.active{font-weight:bold;}h1{color:var(--accent);}form{margin:2em auto;max-width:400px;}label{display:block;margin:1em 0 0.5em;}input[type=text],input[type=password]{width:100%;padding:0.5em;border-radius:4px;border:1px solid var(--border);background:var(--card);color:var(--text);}input[type=checkbox]{margin-right:0.5em;}button{background:var(--accent);color:#fff;padding:0.7em 2em;border:none;border-radius:4px;font-size:1em;margin-top:1em;cursor:pointer;}button:hover{background:#246;}div.card{background:var(--card);padding:1.5em 1em;margin:2em auto;max-width:420px;border-radius:8px;box-shadow:0 2px 8px #0004;}@media (max-width:600px){form,div.card{max-width:98vw;}}</style>"
    "</head><body><nav><a href='/' >Info</a><a href='/wifi' class='active'>WiFi</a></nav>"
    "<div class='container'><h1>WiFi Settings</h1>" + message + "<div class='card'><form method='POST'>"
    "<label><input type='checkbox' name='wifiEnabled' " + checked + ">Enable WiFi</label>"
    "<label>SSID:<input type='text' name='ssid' value='" + wifiSsid + "'></label>"
    "<label>Password:<input type='password' name='pass' value='" + wifiPass + "'></label>"
    "<button type='submit'>Save & Reboot</button>"
    "</form></div></div></body></html>";
  server.send(200, "text/html", html);
}
#include <ArduinoOTA.h>
// ── Web server instance ─────────────────────────────────────────────────────
WebServer server(80);

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>TWatch ESP Info</title>"
    "<style>:root{--bg:#181c20;--card:#23272b;--text:#fff;--accent:#35a;--border:#444;}body{background:var(--bg);color:var(--text);font-family:sans-serif;margin:0;}nav{background:var(--card);padding:1em;text-align:center;}nav a{color:var(--accent);margin:0 1em;text-decoration:none;}nav a.active{font-weight:bold;}h1{color:var(--accent);}table{margin:auto;background:var(--card);border-radius:8px;}td,th{padding:8px 16px;}th{color:var(--accent);}div.card{background:var(--card);padding:1.5em 1em;margin:2em auto;max-width:420px;border-radius:8px;box-shadow:0 2px 8px #0004;}@media (max-width:600px){div.card{max-width:98vw;}}</style>"
    "</head><body><nav><a href='/' class='active'>Info</a><a href='/wifi'>WiFi</a></nav>"
    "<div class='container'><h1>TWatch ESP Info</h1>"
    "<div class='card'><table>"
    "<tr><th>Chip Model</th><td>" + String(ESP.getChipModel()) + "</td></tr>"
    "<tr><th>Chip Revision</th><td>" + String(ESP.getChipRevision()) + "</td></tr>"
    "<tr><th>CPU Freq (MHz)</th><td>" + String(ESP.getCpuFreqMHz()) + "</td></tr>"
    "<tr><th>Flash Size</th><td>" + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB</td></tr>"
    "<tr><th>Sketch Size</th><td>" + String(ESP.getSketchSize() / 1024) + " KB</td></tr>"
    "<tr><th>Free Heap</th><td>" + String(ESP.getFreeHeap() / 1024) + " KB</td></tr>"
    "<tr><th>WiFi SSID</th><td>" + String(WiFi.SSID()) + "</td></tr>"
    "<tr><th>IP Address</th><td>" + WiFi.localIP().toString() + "</td></tr>"
    "</table>"
    "<p style='color:#888'>OTA enabled | " + String(__DATE__) + " " + String(__TIME__) + "</p></div></div></body></html>";
  server.send(200, "text/html", html);
}
  loadWifiSettings();

  // If WiFi is enabled, try to connect. If not connected, start AP mode for config.
  if (wifiEnabled) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000) {
      delay(200);
    }
    if (WiFi.status() != WL_CONNECTED) {
      apMode = true;
    }
  } else {
    apMode = true;
  }

  if (apMode) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("TWatch-Setup", "twatch1234");
  }

  // Arduino OTA setup
  ArduinoOTA.setHostname("twatch-espnow");
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Update Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA Update End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  // Web server setup
  server.on("/", handleRoot);
  server.begin();

  // Handle OTA and web server
  ArduinoOTA.handle();
  server.handleClient();
#include <esp_now.h>
#include <esp_wifi.h>
#include <time.h>
#include <math.h>

TTGOClass    *ttgo        = nullptr;

// ── Config ────────────────────────────────────────────────────────────────────
#define TIME_RIC   224   // POCSAG RIC carrying date/time

// RICs silently processed but never shown on the POCSAG screen
static const uint32_t HIDDEN_RICS[] = { 224, 208, 200, 216 };
static bool isHiddenRic(uint32_t ric) {
  for (size_t i = 0; i < sizeof(HIDDEN_RICS) / sizeof(HIDDEN_RICS[0]); i++)
    if (HIDDEN_RICS[i] == ric) return true;
  return false;
}

// ── Packet types — must match system/system_espnow.h ─────────────────────────
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

// ── State ─────────────────────────────────────────────────────────────────────
#define SCREEN_CLOCK   0
#define SCREEN_POCSAG  1
#define SCREEN_DMR     2
#define SCREEN_COUNT   3

static int           currentScreen    = SCREEN_CLOCK;
static bool          needsRedraw      = true;
static int           lastDrawnSecond  = -1;
static unsigned long lastPacketMillis = 0;
static unsigned long lastAnimMillis   = 0;
#define AUTO_CLOCK_MS  15000
#define ANIM_FRAME_MS  50


static bool displayOn      = true;


// DMR
static uint32_t lastDmrSrc   = 0;
static uint32_t lastDmrDst   = 0;
static uint8_t  lastDmrSlot  = 0;
static bool     lastDmrGroup = true;

// POCSAG
static uint32_t lastRic = 0;
static char     lastMsg[POCSAG_MSG_MAX_LEN + 1] = "";

// Time
static time_t         baseEpoch    = 0;
static unsigned long  baseMillis   = 0;
static bool           espnowSynced = false;

// Previous hand angles — for incremental erase (avoids full-face redraw each second)
static float prevHourAngle   = -999.0f;
static float prevMinuteAngle = -999.0f;
static float prevSecondAngle = -999.0f;

// ── Time helpers ──────────────────────────────────────────────────────────────
static void parseTimePacket(const char* msg) {
  if (strlen(msg) < 26) return;
  const char* p = msg + 14;   // skip 14-char literal prefix
  for (int i = 0; i < 12; i++) {
    if (p[i] < '0' || p[i] > '9') return;
  }
  struct tm t = {};
  t.tm_year  = 100 + (p[0]-'0')*10 + (p[1]-'0');
  t.tm_mon   = (p[2]-'0')*10 + (p[3]-'0') - 1;
  t.tm_mday  = (p[4]-'0')*10 + (p[5]-'0');
  t.tm_hour  = (p[6]-'0')*10 + (p[7]-'0');
  t.tm_min   = (p[8]-'0')*10 + (p[9]-'0');
  t.tm_sec   = (p[10]-'0')*10 + (p[11]-'0');
  t.tm_isdst = -1;

  baseEpoch    = mktime(&t);
  baseMillis   = millis();
  espnowSynced = true;

  Serial.printf("[TIME] synced: %04d-%02d-%02d %02d:%02d:%02d\n",
                t.tm_year+1900, t.tm_mon+1, t.tm_mday,
                t.tm_hour, t.tm_min, t.tm_sec);

  // Persist to RTC (PCF8563) — single RTC_Date holds date + time
  if (ttgo) {
    ttgo->rtc->setDateTime(
      RTC_Date(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
               t.tm_hour, t.tm_min, t.tm_sec)
    );
  }
}

static bool getClockTime(struct tm* t) {
  if (baseEpoch == 0) return false;
  time_t now = baseEpoch + (time_t)((millis() - baseMillis) / 1000UL);
  localtime_r(&now, t);
  return true;
}

// ── Dark square-face clock (Style Seven inspired) ─────────────────────────────
// Black face, white ticks + numerals, royal-blue hands, red second hand.
// Background never changes → zero flicker (erase = draw in black).
#define CLK_BG    TFT_BLACK   // face background
#define CLK_TICK  TFT_WHITE   // tick marks and hour numerals
#define CLK_HAND  0x435B      // royal blue (#4169E1) — hour + minute hands
#define CLK_DATE  0x435B      // date box border and text
#define TICK_OUTER 113        // outer radius of tick ring (fits 240px screen)
#define NUM_RADIUS  88        // radius to center of hour numerals

static void fillHand(float cx, float cy, float angleDeg,
                     float front, float back, float width, uint32_t color) {
  float rad  = (angleDeg - 90.0f) * DEG_TO_RAD;
  float cosA = cosf(rad), sinA = sinf(rad);
  float px   = -sinA, py = cosA;
  float hw   = width / 2.0f;

  float x0 = cx - back*cosA + hw*px,  y0 = cy - back*sinA + hw*py;
  float x1 = cx - back*cosA - hw*px,  y1 = cy - back*sinA - hw*py;
  float x2 = cx + front*cosA + hw*px, y2 = cy + front*sinA + hw*py;
  float x3 = cx + front*cosA - hw*px, y3 = cy + front*sinA - hw*py;

  ttgo->tft->fillTriangle(x0, y0, x1, y1, x2, y2, color);
  ttgo->tft->fillTriangle(x1, y1, x2, y2, x3, y3, color);
}

static const char* const HOUR_LABEL[12] = {
  "12","1","2","3","4","5","6","7","8","9","10","11"
};

static void drawHourNum(int idx) {
  float angle = (idx * 30.0f - 90.0f) * DEG_TO_RAD;
  int x = 120 + (int)(NUM_RADIUS * cosf(angle));
  int y = 120 + (int)(NUM_RADIUS * sinf(angle));
  ttgo->tft->setTextDatum(MC_DATUM);
  ttgo->tft->setTextFont(4);
  ttgo->tft->setTextColor(CLK_TICK, CLK_BG);
  ttgo->tft->drawString(HOUR_LABEL[idx], x, y);
}

// 60 tick marks: hour ticks are thicker (3 parallel lines), minute ticks single
static void drawTicks() {
  for (int i = 0; i < 60; i++) {
    bool  isHour = (i % 5 == 0);
    float angle  = (i * 6.0f - 90.0f) * DEG_TO_RAD;
    int   len    = isHour ? 13 : 6;
    float ca = cosf(angle), sa = sinf(angle);
    int x1 = 120 + (int)(TICK_OUTER * ca),          y1 = 120 + (int)(TICK_OUTER * sa);
    int x2 = 120 + (int)((TICK_OUTER-len) * ca),    y2 = 120 + (int)((TICK_OUTER-len) * sa);
    ttgo->tft->drawLine(x1, y1, x2, y2, CLK_TICK);
    if (isHour) {
      int px = -(int)(sa + 0.5f), py = (int)(ca + 0.5f);
      ttgo->tft->drawLine(x1+px, y1+py, x2+px, y2+py, CLK_TICK);
      ttgo->tft->drawLine(x1-px, y1-py, x2-px, y2-py, CLK_TICK);
    }
  }
}

// Full static face: ticks + numerals (called once on first show)
static void drawClockStaticFace() {
  drawTicks();
  for (int i = 0; i < 12; i++) drawHourNum(i);
}

// Two date boxes, Style Seven style
static void drawDateBoxes(const struct tm* t) {
  static const char* const MON[] = {
    "JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};
  static const char* const DOW[] = {
    "SUN","MON","TUE","WED","THU","FRI","SAT"};

  // Left box: month / day-of-week (between 9 and center)
  const int lx=20, ly=102, lw=74, lh=38;
  ttgo->tft->fillRect(lx, ly, lw, lh, CLK_BG);
  ttgo->tft->drawRect(lx, ly, lw, lh, CLK_DATE);
  ttgo->tft->drawFastHLine(lx+1, ly+19, lw-2, CLK_DATE);
  ttgo->tft->setTextDatum(MC_DATUM);
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(CLK_DATE, CLK_BG);
  ttgo->tft->drawString(MON[t->tm_mon],  lx + lw/2, ly + 10);
  ttgo->tft->drawString(DOW[t->tm_wday], lx + lw/2, ly + 29);

  // Right box: day number (between center and 3)
  const int rx=150, ry=104, rw=68, rh=34;
  char buf[4];
  snprintf(buf, sizeof(buf), "%02d", t->tm_mday);
  ttgo->tft->fillRect(rx, ry, rw, rh, CLK_BG);
  ttgo->tft->drawRect(rx, ry, rw, rh, CLK_DATE);
  ttgo->tft->setTextDatum(MC_DATUM);
  ttgo->tft->setTextFont(4);
  ttgo->tft->setTextColor(CLK_DATE, CLK_BG);
  ttgo->tft->drawString(buf, rx + rw/2, ry + rh/2);
}

// Erase old hands (draw in black), then restore ticks + numerals + date boxes
static void eraseClockHands(const struct tm* t) {
  if (prevHourAngle < -900) return;
  const int CX = 120, CY = 120;

  fillHand(CX, CY, prevHourAngle,   55, 14, 13, CLK_BG);
  fillHand(CX, CY, prevMinuteAngle, 78, 14, 10, CLK_BG);

  float sRad = (prevSecondAngle - 90.0f) * DEG_TO_RAD;
  int tipX  = CX + (int)(85 * cosf(sRad));
  int tipY  = CY + (int)(85 * sinf(sRad));
  int tailX = CX - (int)(20 * cosf(sRad));
  int tailY = CY - (int)(20 * sinf(sRad));
  for (int d = -2; d <= 2; d++) {
    ttgo->tft->drawLine(tailX+d, tailY, tipX+d, tipY, CLK_BG);
    ttgo->tft->drawLine(tailX, tailY+d, tipX, tipY+d, CLK_BG);
  }
  ttgo->tft->fillCircle(CX, CY, 9, CLK_BG);

  // Restore static face elements swept by hands
  drawTicks();
  for (int i = 0; i < 12; i++) drawHourNum(i);
  if (t) drawDateBoxes(t);
}

static void drawClockHands(int h, int m, int s) {
  const int CX = 120, CY = 120;

  prevHourAngle   = (h % 12) * 30.0f + m * 0.5f;
  prevMinuteAngle = m * 6.0f + s * 0.1f;
  prevSecondAngle = s * 6.0f;

  fillHand(CX, CY, prevHourAngle,   55, 14, 10, CLK_HAND);
  fillHand(CX, CY, prevMinuteAngle, 78, 14,  7, CLK_HAND);

  // Second hand — thin red line
  float sRad = (prevSecondAngle - 90.0f) * DEG_TO_RAD;
  int tipX  = CX + (int)(85 * cosf(sRad));
  int tipY  = CY + (int)(85 * sinf(sRad));
  int tailX = CX - (int)(20 * cosf(sRad));
  int tailY = CY - (int)(20 * sinf(sRad));
  ttgo->tft->drawLine(tailX,   tailY,   tipX,   tipY,   TFT_RED);
  ttgo->tft->drawLine(tailX+1, tailY,   tipX+1, tipY,   TFT_RED);
  ttgo->tft->drawLine(tailX,   tailY+1, tipX,   tipY+1, TFT_RED);

  // Center: blue circle with white centre dot
  ttgo->tft->fillCircle(CX, CY, 7, CLK_HAND);
  ttgo->tft->fillCircle(CX, CY, 3, TFT_WHITE);
}

// ── Page dots (POCSAG + DMR screens) ──────────────────────────────────────────
static void drawPageDots() {
  int cx = 120, y = 220;
  for (int i = 0; i < SCREEN_COUNT; i++) {
    int x = cx + (i - 1) * 12;
    if (i == currentScreen)
      ttgo->tft->fillCircle(x, y, 4, TFT_WHITE);
    else
      ttgo->tft->drawCircle(x, y, 4, TFT_DARKGREY);
  }
}

// ── Screen draw functions ──────────────────────────────────────────────────────
static void drawClockDots() {
  for (int i = 0; i < SCREEN_COUNT; i++) {
    int x = 120 + (i - 1) * 12;
    if (i == currentScreen)
      ttgo->tft->fillCircle(x, 225, 3, CLK_HAND);
    else
      ttgo->tft->drawCircle(x, 225, 3, TFT_DARKGREY);
  }
}

static void drawClockScreen() {
  struct tm t;
  bool hasTime = getClockTime(&t);

  if (lastDrawnSecond == -1) {
    // First show: black fill + static face + date + overlay indicators
    ttgo->tft->fillScreen(CLK_BG);
    drawClockStaticFace();
    if (hasTime) drawDateBoxes(&t);
    if (espnowSynced) ttgo->tft->fillCircle(120, 8, 4, CLK_HAND);
    drawClockDots();
    prevHourAngle = prevMinuteAngle = prevSecondAngle = -999.0f;
  } else {
    // Incremental: erase old hands + restore what they covered
    eraseClockHands(hasTime ? &t : nullptr);
    if (espnowSynced) ttgo->tft->fillCircle(120, 8, 4, CLK_HAND);
    drawClockDots();
  }

  if (hasTime) {
    drawClockHands(t.tm_hour, t.tm_min, t.tm_sec);
    lastDrawnSecond = t.tm_sec;
  } else {
    // Pre-sync: fast-spinning hands (20× speed)
    time_t fakeTime = (time_t)(millis() / 50UL);
    struct tm f;
    gmtime_r(&fakeTime, &f);
    drawClockHands(f.tm_hour, f.tm_min, f.tm_sec);
    lastDrawnSecond = f.tm_sec;
  }
}

static void drawPocsagScreen() {
  ttgo->tft->fillScreen(TFT_BLACK);
  ttgo->tft->setTextDatum(TC_DATUM);

  ttgo->tft->setTextColor(TFT_ORANGE, TFT_BLACK);
  ttgo->tft->setTextFont(2);
  ttgo->tft->drawString("POCSAG", 120, 18);
  ttgo->tft->drawFastHLine(50, 38, 140, TFT_DARKGREY);

  if (lastRic > 0) {
    ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
    ttgo->tft->setTextFont(4);
    String msg = String(lastMsg);
    int y = 52;
    for (int offset = 0; offset < (int)msg.length() && offset < 27; offset += 9) {
      ttgo->tft->drawString(msg.substring(offset, offset + 9), 120, y);
      y += 32;
    }
    char ricLine[20];
    snprintf(ricLine, sizeof(ricLine), "RIC %u", lastRic);
    ttgo->tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    ttgo->tft->setTextFont(2);
    ttgo->tft->drawString(ricLine, 120, 168);
  } else {
    ttgo->tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    ttgo->tft->setTextFont(2);
    ttgo->tft->drawString("no data", 120, 110);
  }

  drawPageDots();
}

static void drawDmrScreen() {
  ttgo->tft->fillScreen(TFT_BLACK);
  ttgo->tft->setTextDatum(TC_DATUM);

  ttgo->tft->setTextColor(TFT_GREEN, TFT_BLACK);
  ttgo->tft->setTextFont(2);
  ttgo->tft->drawString("DMR", 120, 18);
  ttgo->tft->drawFastHLine(50, 38, 140, TFT_DARKGREY);

  if (lastDmrSrc > 0) {
    ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
    ttgo->tft->setTextFont(4);
    ttgo->tft->drawString(String(lastDmrSrc), 120, 52);

    ttgo->tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    ttgo->tft->setTextFont(2);
    ttgo->tft->drawString("to", 120, 94);

    ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
    ttgo->tft->setTextFont(4);
    ttgo->tft->drawString(String(lastDmrDst), 120, 112);

    char info[16];
    snprintf(info, sizeof(info), "TS%u  %s", lastDmrSlot, lastDmrGroup ? "TG" : "PC");
    ttgo->tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    ttgo->tft->setTextFont(2);
    ttgo->tft->drawString(info, 120, 162);
  } else {
    ttgo->tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    ttgo->tft->setTextFont(2);
    ttgo->tft->drawString("no data", 120, 110);
  }

  drawPageDots();
}

static void redraw() {
  switch (currentScreen) {
    case SCREEN_CLOCK:  drawClockScreen();  break;
    case SCREEN_POCSAG: drawPocsagScreen(); break;
    case SCREEN_DMR:    drawDmrScreen();    break;
  }
}

// Short vibration buzz on packet receive (GPIO4 motor)
static void vibrate(int ms = 80) {
  if (ttgo) ttgo->motor->onec(ms);
}

// ── ESP-NOW receive callback (arduino-esp32 3.x signature) ───────────────────
static void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  const uint8_t *mac = info->src_addr;
  if (len < 2) return;

  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

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
    vibrate();
    Serial.printf("[DMR] from %s  src=%u dst=%u TS%u %s\n",
                  macStr, lastDmrSrc, lastDmrDst, lastDmrSlot, lastDmrGroup ? "TG" : "PC");
  }
  else if (data[0] == ESPNOW_TYPE_POCSAG) {
    EspNowPocsagPacket pkt = {};
    memcpy(&pkt, data, min(len, (int)sizeof(pkt)));
    pkt.message[POCSAG_MSG_MAX_LEN] = '\0';

    Serial.printf("[POCSAG] from %s  RIC=%u func=%u msg=\"%s\"\n",
                  macStr, pkt.ric, pkt.functional, pkt.message);

    if (pkt.ric == TIME_RIC) {
      parseTimePacket(pkt.message);
      if (currentScreen == SCREEN_CLOCK) needsRedraw = true;
    }

    if (!isHiddenRic(pkt.ric)) {
      lastRic = pkt.ric;
      strncpy(lastMsg, pkt.message, POCSAG_MSG_MAX_LEN);
      lastMsg[POCSAG_MSG_MAX_LEN] = '\0';
      currentScreen    = SCREEN_POCSAG;
      lastPacketMillis = millis();
      needsRedraw      = true;
      vibrate();
    }
  }
}

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== twatch-espnow ===");

  ttgo = TTGOClass::getWatch();
  ttgo->begin();
  ttgo->openBL();
  ttgo->motor_begin();   // vibration motor on GPIO4

  ttgo->tft->setRotation(0);
  ttgo->tft->fillScreen(TFT_BLACK);

  // Seed from RTC (PCF8563) if it holds a valid time
  // RTC_Date carries both date and time fields
  RTC_Date dt = ttgo->rtc->getDateTime();
  if (dt.year >= 2023) {
    struct tm t = {};
    t.tm_year  = dt.year - 1900;
    t.tm_mon   = dt.month - 1;
    t.tm_mday  = dt.day;
    t.tm_hour  = dt.hour;
    t.tm_min   = dt.minute;
    t.tm_sec   = dt.second;
    t.tm_isdst = -1;
    baseEpoch  = mktime(&t);
    baseMillis = millis();
    Serial.printf("[RTC] loaded: %04d-%02d-%02d %02d:%02d:%02d\n",
                  dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
  } else {
    Serial.println("[RTC] no valid time stored yet");
  }



  server.on("/wifi", handleWifiConfig);
  // ...existing code...

  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init FAILED");
    ttgo->tft->fillScreen(TFT_BLACK);
    ttgo->tft->setTextDatum(MC_DATUM);
    ttgo->tft->setTextColor(TFT_RED, TFT_BLACK);
    ttgo->tft->setTextFont(2);
    ttgo->tft->drawString("ESP-NOW FAILED", 120, 120);
    return;
  }
  esp_now_register_recv_cb(onReceive);
  Serial.println("ESP-NOW ready");

  needsRedraw = true;
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {

  // Touch tap: wake display if off, otherwise advance screen
  static bool wasTouched = false;
  int16_t tx, ty;
  bool isTouched = ttgo->getTouch(tx, ty);
  if (isTouched && !wasTouched) {
    if (!displayOn) {
      displayOn       = true;
      ttgo->openBL();
      lastDrawnSecond = -1;
      needsRedraw     = true;
    } else {
      currentScreen   = (currentScreen + 1) % SCREEN_COUNT;
      lastDrawnSecond = -1;
      needsRedraw     = true;
    }
  }
  wasTouched = isTouched;

  // Skip redraws while display is off
  if (!displayOn) {
    delay(50);
    return;
  }

  // Auto-return to clock after 15 s of no new packets
  if (currentScreen != SCREEN_CLOCK && lastPacketMillis > 0 &&
      millis() - lastPacketMillis >= AUTO_CLOCK_MS) {
    lastPacketMillis = 0;
    currentScreen    = SCREEN_CLOCK;
    lastDrawnSecond  = -1;
    needsRedraw      = true;
  }

  // Clock screen: redraw every second when synced, every 50 ms while spinning
  if (currentScreen == SCREEN_CLOCK && !needsRedraw) {
    if (baseEpoch == 0) {
      if (millis() - lastAnimMillis >= ANIM_FRAME_MS) {
        lastAnimMillis = millis();
        needsRedraw = true;
      }
    } else {
      struct tm t;
      if (getClockTime(&t) && t.tm_sec != lastDrawnSecond)
        needsRedraw = true;
    }
  }

  if (needsRedraw) {
    needsRedraw = false;
    redraw();
  }

  delay(50);
}
