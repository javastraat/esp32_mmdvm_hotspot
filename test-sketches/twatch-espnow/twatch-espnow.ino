/*
 * twatch-espnow.ino  —  ESP-NOW receiver for TTGO T-Watch 2020
 *
 * Four screens, tap the touchscreen to cycle:
 *   Screen 0 — CLOCK:    Mondaine-style analog clock (time from RIC 224)
 *   Screen 1 — POCSAG:   message (wrapped), RIC
 *   Screen 2 — DMR:      src → dst, slot, TG/PC
 *   Screen 3 — SETTINGS: 3×3 grid; tap WiFi cell → WIFI sub-screen
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
#include "config.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <FS.h>
using fs::FS;
#include <WebServer.h>
#include <time.h>
#include <math.h>

TTGOClass *ttgo = nullptr;

// ── Config — runtime values, loaded from NVS (defaults from config.h) ─────────
static uint32_t timeRic        = DEFAULT_TIME_RIC;
static String   deviceHostname = DEFAULT_HOSTNAME;
static uint8_t  watchfaceId    = DEFAULT_WATCHFACE;   // 0=Analog 1=Digital
static bool     clock24h       = DEFAULT_CLOCK_24H;

// Hidden RICs — loaded from NVS, editable via web UI
static uint32_t hiddenRics[MAX_HIDDEN_RICS];
static int      hiddenRicCount = 0;

static bool isHiddenRic(uint32_t ric) {
  for (int i = 0; i < hiddenRicCount; i++)
    if (hiddenRics[i] == ric) return true;
  return false;
}

static void parseHiddenRicsStr(const char* str) {
  hiddenRicCount = 0;
  char buf[96];
  strncpy(buf, str, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  char* tok = strtok(buf, ",");
  while (tok && hiddenRicCount < MAX_HIDDEN_RICS) {
    long v = atol(tok);
    if (v > 0) hiddenRics[hiddenRicCount++] = (uint32_t)v;
    tok = strtok(nullptr, ",");
  }
}

static String hiddenRicsToStr() {
  String s;
  for (int i = 0; i < hiddenRicCount; i++) {
    if (i > 0) s += ',';
    s += String(hiddenRics[i]);
  }
  return s;
}

// Quick-settings state (declared here so saveBrightnessVibe/loadConfig can use them)
static uint8_t       currentBrightness = 180;
static bool          vibeEnabled       = true;

// Save brightness + vibe to NVS (called from quick-settings overlay)
static void saveBrightnessVibe() {
  Preferences p;
  p.begin("config", false);
  p.putUInt("brightness", currentBrightness);
  p.putBool("vibeEnabled", vibeEnabled);
  p.end();
}

static void loadConfig() {
  Preferences p;
  p.begin("config", true);
  String storedRics  = p.getString("hiddenRics", "");
  timeRic            = p.getUInt  ("timeRic",    DEFAULT_TIME_RIC);
  deviceHostname     = p.getString("hostname",   DEFAULT_HOSTNAME);
  watchfaceId        = (uint8_t)p.getUInt("watchface", DEFAULT_WATCHFACE);
  clock24h           = p.getBool("clock24h", DEFAULT_CLOCK_24H);
  currentBrightness  = (uint8_t)p.getUInt("brightness", 180);
  vibeEnabled        = p.getBool("vibeEnabled", true);
  p.end();

  if (storedRics.length() == 0) {
    // First boot — write hidden RIC defaults to NVS
    p.begin("config", false);
    p.putString("hiddenRics", DEFAULT_HIDDEN_RICS);
    p.end();
    storedRics = DEFAULT_HIDDEN_RICS;
  }
  parseHiddenRicsStr(storedRics.c_str());
  Serial.printf("[config] hiddenRics: %s\n", storedRics.c_str());
  Serial.printf("[config] timeRic: %u\n", timeRic);
  Serial.printf("[config] hostname: %s\n", deviceHostname.c_str());
  Serial.printf("[config] clock24h: %s\n", clock24h ? "true" : "false");
}

static void saveHiddenRics(const String& csv) {
  parseHiddenRicsStr(csv.c_str());
  Preferences p;
  p.begin("config", false);
  p.putString("hiddenRics", hiddenRicsToStr());
  p.end();
}

static void saveTimeRic(uint32_t ric) {
  timeRic = ric;
  Preferences p;
  p.begin("config", false);
  p.putUInt("timeRic", ric);
  p.end();
}

static void saveHostname(const String& h) {
  deviceHostname = h;
  Preferences p;
  p.begin("config", false);
  p.putString("hostname", h);
  p.end();
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

// ── Screen identifiers ────────────────────────────────────────────────────────
#define SCREEN_CLOCK         0
#define SCREEN_POCSAG        1
#define SCREEN_DMR           2
#define SCREEN_SETTINGS      3
#define SCREEN_WIFI          4
#define SCREEN_PAGER         5
#define SCREEN_BATTERY       6
#define SCREEN_WATCHFACE     7
#define SCREEN_QUICKSETTINGS 8
#define SCREEN_STEPS         9
#define SCREEN_STOPWATCH     10
#define SCREEN_COUNT         4   // number of screens in the tap-cycle (sub-screens excluded)

// ── Global state ──────────────────────────────────────────────────────────────
static int           currentScreen    = SCREEN_CLOCK;
static int           prevScreen       = SCREEN_CLOCK;
static bool          needsRedraw      = true;
static int           lastDrawnSecond  = -1;
static unsigned long lastPacketMillis   = 0;
static unsigned long lastAnimMillis     = 0;
static unsigned long lastActivityMillis = 0;
#define AUTO_CLOCK_MS  15000
#define ANIM_FRAME_MS  50

static bool          displayOn        = true;
static volatile bool axpIrq           = false;  // set by AXP202 interrupt
static volatile bool bmaIrq           = false;  // set by BMA423 interrupt

// DMR
static uint32_t lastDmrSrc   = 0;
static uint32_t lastDmrDst   = 0;
static uint8_t  lastDmrSlot  = 0;
static bool     lastDmrGroup = true;

// POCSAG — last message shown on screen
static uint32_t lastRic = 0;
static char     lastMsg[POCSAG_MSG_MAX_LEN + 1] = "";

// POCSAG message log — ALL received messages, newest first (ring buffer, 5 entries)
struct PocLogEntry {
  uint32_t ric;
  char     msg[POCSAG_MSG_MAX_LEN + 1];
  char     timeStr[24];
};
static PocLogEntry pocLog[5];
static int         pocLogHead  = 0;   // next write slot
static int         pocLogCount = 0;   // valid entries (0–5)

// Time
static time_t         baseEpoch    = 0;
static unsigned long  baseMillis   = 0;
static bool           espnowSynced = false;

// Previous hand angles — for incremental erase (avoids full-face redraw each second)
static float prevHourAngle   = -999.0f;
static float prevMinuteAngle = -999.0f;
static float prevSecondAngle = -999.0f;

static void saveWatchfaceSettings(uint8_t id, bool use24h) {
  watchfaceId = id;
  clock24h    = use24h;
  Preferences p;
  p.begin("config", false);
  p.putUInt("watchface", id);
  p.putBool("clock24h", use24h);
  p.end();
  lastDrawnSecond = -1;   // force full redraw on next clock show
}

// WiFi / online-mode preference
Preferences  modePrefs;
bool         onlineMode       = true;
static bool  modeRebootPrompt = false;
static bool  settingsRebootPrompt = false;

void loadOnlineMode() {
  modePrefs.begin("settings", true);
  onlineMode = modePrefs.getBool("onlineMode", true);
  modePrefs.end();
}

void saveOnlineMode() {
  modePrefs.begin("settings", false);
  modePrefs.putBool("onlineMode", onlineMode);
  modePrefs.end();
}

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

  // Persist to RTC (PCF8563)
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

// ── Screen includes ───────────────────────────────────────────────────────────
// Included after all globals so each header can access them directly.
#include "screens/clock_screen.h"
#include "screens/pocsag_screen.h"
#include "screens/dmr_screen.h"
#include "screens/settings_screen.h"
#include "screens/wifi_screen.h"
#include "screens/pager_screen.h"
#include "screens/battery_screen.h"
#include "screens/watchface_screen.h"
#include "screens/ota_screen.h"
#include "screens/quicksettings_screen.h"
#include "screens/steps_screen.h"
#include "screens/stopwatch_screen.h"
#include "web/main.h"

// ── Dispatch ──────────────────────────────────────────────────────────────────
static void redraw() {
  switch (currentScreen) {
    case SCREEN_CLOCK:    drawClockScreen();        break;
    case SCREEN_POCSAG:   drawPocsagScreen();       break;
    case SCREEN_DMR:      drawDmrScreen();          break;
    case SCREEN_SETTINGS: drawSettingsScreen();     break;
    case SCREEN_WIFI:     drawWifiSettingsScreen(); break;
    case SCREEN_PAGER:    drawPagerScreen();        break;
    case SCREEN_BATTERY:       drawBatteryScreen();       break;
    case SCREEN_WATCHFACE:     drawWatchfaceScreen();     break;
    case SCREEN_QUICKSETTINGS: drawQuickSettingsScreen(); break;
    case SCREEN_STEPS:         drawStepsScreen();         break;
    case SCREEN_STOPWATCH:     drawStopwatchScreen();     break;
  }
}

// Short vibration buzz on packet receive (GPIO4 motor)
static void vibrate(int ms = 80) {
  if (ttgo && vibeEnabled) ttgo->motor->onec(ms);
}

// Wake the display and reset the inactivity timer (called on incoming packets)
static void wakeScreen() {
  if (!displayOn) {
    displayOn = true;
    if (ttgo) {
      ttgo->openBL();
      ttgo->bl->adjust(currentBrightness);
    }
  }
  lastActivityMillis = millis();
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

    if (pkt.ric == timeRic) {
      parseTimePacket(pkt.message);
      if (currentScreen == SCREEN_CLOCK) needsRedraw = true;
    }

    // Log ALL messages to the ring buffer (web dashboard)
    {
      PocLogEntry& e = pocLog[pocLogHead];
      e.ric = pkt.ric;
      strncpy(e.msg, pkt.message, POCSAG_MSG_MAX_LEN);
      e.msg[POCSAG_MSG_MAX_LEN] = '\0';
      struct tm lt;
      if (getClockTime(&lt))
        snprintf(e.timeStr, sizeof(e.timeStr), "%02d:%02d:%02d", lt.tm_hour, lt.tm_min, lt.tm_sec);
      else
        strcpy(e.timeStr, "--:--:--");
      pocLogHead = (pocLogHead + 1) % 5;
      if (pocLogCount < 5) pocLogCount++;
    }

    // Only show on watch display if RIC is NOT in the hidden list
    if (!isHiddenRic(pkt.ric)) {
      lastRic = pkt.ric;
      strncpy(lastMsg, pkt.message, POCSAG_MSG_MAX_LEN);
      lastMsg[POCSAG_MSG_MAX_LEN] = '\0';
      currentScreen    = SCREEN_POCSAG;
      lastPacketMillis = millis();
      needsRedraw      = true;
      wakeScreen();
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

  loadConfig();          // load brightness + vibe before applying them
  ttgo->bl->adjust(currentBrightness);

  // BMA423 accelerometer — step counter + raise-to-wake
  // Note: ttgo->begin() already initialises the BMA; configure it here.
  {
    Acfg cfg;
    cfg.odr       = BMA4_OUTPUT_DATA_RATE_100HZ;
    cfg.range     = BMA4_ACCEL_RANGE_2G;
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;
    ttgo->bma->accelConfig(cfg);
  }
  ttgo->bma->enableAccel();                               // MUST be before enableFeature
  ttgo->bma->enableFeature(BMA423_STEP_CNTR, true);      // pedometer
  ttgo->bma->enableFeature(BMA423_TILT,      true);      // raise-to-wake (wear correctly)
  ttgo->bma->enableFeature(BMA423_WAKEUP,    true);      // double-tap to wake
  ttgo->bma->resetStepCounter();
  ttgo->bma->enableStepCountInterrupt();
  ttgo->bma->enableTiltInterrupt();
  ttgo->bma->enableWakeupInterrupt();
  pinMode(BMA423_INT1, INPUT);
  attachInterrupt(BMA423_INT1, []{ bmaIrq = true; }, RISING);

  // Enable AXP202 ADC channels so battery/USB readings work
  ttgo->power->adc1Enable(
      AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1 |
      AXP202_BATT_CUR_ADC1 | AXP202_BATT_VOL_ADC1, true);
  ttgo->power->EnableCoulombcounter();

  // AXP IRQ pin is input-only on this board; rely on external board pull-up.
  pinMode(AXP202_INT, INPUT);
  attachInterrupt(AXP202_INT, []{ axpIrq = true; }, FALLING);
  ttgo->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ | AXP202_PEK_LONGPRESS_IRQ, true);
  ttgo->power->clearIRQ();

  loadOnlineMode();   // restore persisted WiFi mode from NVS
  // Note: loadConfig() already called above (before bl->adjust)

  ttgo->tft->setRotation(0);
  ttgo->tft->fillScreen(TFT_BLACK);

  // Seed from RTC (PCF8563) if it holds a valid time
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

  if (!onlineMode) {
    // Offline mode: use ESP-NOW (no AP connection needed)
    WiFi.mode(WIFI_STA);
    delay(100);

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
  } else {
    // Online mode: connect to WiFi
    Serial.printf("Connecting to %s ...\n", DEFAULT_WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASS);

    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
      delay(250);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("WiFi connected — IP: %s\n", WiFi.localIP().toString().c_str());

      ArduinoOTA.setHostname(deviceHostname.c_str());
      ArduinoOTA.setPassword("mmdvm");
      ArduinoOTA
        .onStart([]() {
          Serial.println("OTA start");
          otaStart();
        })
        .onProgress([](unsigned int done, unsigned int total) {
          otaProgress(done, total);
        })
        .onEnd([]() {
          Serial.println("\nOTA done");
          otaDone();
        })
        .onError([](ota_error_t e) {
          Serial.printf("OTA error[%u]\n", e);
          otaError(e);
        });
      ArduinoOTA.begin();
      Serial.println("OTA ready");
      setupWebServer();

      // ESP-NOW works alongside WiFi STA — init here so DAPNET packets are received
      if (esp_now_init() == ESP_OK) {
        esp_now_register_recv_cb(onReceive);
        Serial.println("ESP-NOW ready (online mode)");
      } else {
        Serial.println("ESP-NOW init FAILED (online mode)");
      }
      
    } else {
      Serial.println("WiFi connect FAILED (timeout)");
      ttgo->tft->fillScreen(TFT_BLACK);
      ttgo->tft->setTextDatum(MC_DATUM);
      ttgo->tft->setTextColor(TFT_RED, TFT_BLACK);
      ttgo->tft->setTextFont(2);
      ttgo->tft->drawString("WiFi FAILED",      120, 110);
      ttgo->tft->drawString(DEFAULT_WIFI_SSID,   120, 135);
    }
  }

  needsRedraw        = true;
  lastActivityMillis = millis();
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {
  if (onlineMode && WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
    webServer.handleClient();
  }

  // Crown button via AXP202 IRQ
  if (axpIrq) {
    axpIrq = false;
    ttgo->power->readIRQ();

    if (ttgo->power->isPEKShortPressIRQ()) {
      if (currentScreen == SCREEN_QUICKSETTINGS) {
        // Short press on quick-settings → go back
        currentScreen   = prevScreen;
        lastDrawnSecond = -1;
        needsRedraw     = true;
      } else {
        displayOn = !displayOn;
        if (displayOn) {
          ttgo->openBL();
          ttgo->bl->adjust(currentBrightness);
          lastDrawnSecond    = -1;
          lastActivityMillis = millis();
          needsRedraw        = true;
        } else {
          ttgo->closeBL();
        }
      }
    }

    if (ttgo->power->isPEKLongtPressIRQ()) {
      // Long hold → open quick-settings screen
      if (!displayOn) {
        displayOn = true;
        ttgo->openBL();
        ttgo->bl->adjust(currentBrightness);
        lastDrawnSecond = -1;
      }
      prevScreen         = currentScreen;
      currentScreen      = SCREEN_QUICKSETTINGS;
      lastActivityMillis = millis();
      needsRedraw        = true;
    }

    ttgo->power->clearIRQ();
  }

  // BMA423 interrupt — raise-to-wake (tilt) + step counter update
  if (bmaIrq) {
    bmaIrq = false;
    bool rlst;
    do { rlst = ttgo->bma->readInterrupt(); } while (!rlst);
    if (ttgo->bma->isTilt() || ttgo->bma->isDoubleClick()) {
      wakeScreen();
      needsRedraw = true;
    }
    if (ttgo->bma->isStepCounter() && currentScreen == SCREEN_STEPS)
      needsRedraw = true;
  }

  // Touch tap: handle per screen
  static bool wasTouched = false;
  int16_t tx, ty;
  bool isTouched = ttgo->getTouch(tx, ty);
  if (isTouched && !wasTouched) {
    lastActivityMillis = millis();
    if (!displayOn) {
      displayOn       = true;
      ttgo->openBL();
      ttgo->bl->adjust(currentBrightness);
      lastDrawnSecond = -1;
      needsRedraw     = true;
    } else {
      if (currentScreen == SCREEN_QUICKSETTINGS) {
        quickSettingsHandleTouch(tx, ty);
      } else if (currentScreen == SCREEN_SETTINGS) {
        if (settingsRebootPrompt) {
          // YES: left button (x 10-110, y 140-220)
          if (tx >= 10 && tx <= 110 && ty >= 140 && ty <= 220) {
            ESP.restart();
          }
          // NO: right button (x 130-230, y 140-220)
          if (tx >= 130 && tx <= 230 && ty >= 140 && ty <= 220) {
            settingsRebootPrompt = false;
            needsRedraw          = true;
          }
          wasTouched = isTouched;
          return;
        }

        // 3x3 grid: 80x80 cells
        int col = tx / 80;
        int row = ty / 80;
        if (col < 0) col = 0; if (col > 2) col = 2;
        if (row < 0) row = 0; if (row > 2) row = 2;
        if (row == 0 && col == 0) {
          currentScreen = SCREEN_WIFI;
          needsRedraw   = true;
        } else if (row == 0 && col == 1) {
          pagerResetDetailView();
          currentScreen = SCREEN_PAGER;
          needsRedraw   = true;
        } else if (row == 0 && col == 2) {
          batPage       = 0;
          currentScreen = SCREEN_BATTERY;
          needsRedraw   = true;
        } else if (row == 1 && col == 0) {
          watchfaceEditBegin();
          currentScreen = SCREEN_WATCHFACE;
          needsRedraw   = true;
        } else if (row == 1 && col == 1) {
          currentScreen = SCREEN_STEPS;
          needsRedraw   = true;
        } else if (row == 1 && col == 2) {
          currentScreen = SCREEN_STOPWATCH;
          needsRedraw   = true;
        } else if (row == 2 && col == 1) {
          settingsRebootPrompt = true;
          needsRedraw          = true;
        } else if (row == 2 && col == 2) {
          currentScreen = SCREEN_CLOCK;
          needsRedraw   = true;
        } else {
          vibrate();   // placeholder for unimplemented cells
        }
      } else if (currentScreen == SCREEN_WIFI) {
        if (modeRebootPrompt) {
          // YES: left button (x 10-110, y 140-220)
          if (tx >= 10 && tx <= 110 && ty >= 140 && ty <= 220) {
            ESP.restart();
          }
          // NO: right button (x 130-230, y 140-220)
          if (tx >= 130 && tx <= 230 && ty >= 140 && ty <= 220) {
            modeRebootPrompt = false;
            needsRedraw      = true;
          }
        } else {
          // Toggle switch
          int x = 120, y = 158, w = 100, h = 40;
          if (tx >= x-w/2 && tx <= x+w/2 && ty >= y-h/2 && ty <= y+h/2) {
            onlineMode = !onlineMode;
            saveOnlineMode();
            modeRebootPrompt = true;
            needsRedraw      = true;
          }
          // Back button
          if (tx >= 60 && tx <= 180 && ty >= 200 && ty <= 232) {
            currentScreen = SCREEN_SETTINGS;
            loadOnlineMode();
            needsRedraw   = true;
          }
        }
      } else if (currentScreen == SCREEN_PAGER) {
        pagerHandleTouch(tx, ty);
      } else if (currentScreen == SCREEN_BATTERY) {
        batteryHandleTouch(tx, ty);
      } else if (currentScreen == SCREEN_WATCHFACE) {
        watchfaceHandleTouch(tx, ty);
      } else if (currentScreen == SCREEN_STEPS) {
        stepsHandleTouch(tx, ty);
      } else if (currentScreen == SCREEN_STOPWATCH) {
        stopwatchHandleTouch(tx, ty);
      } else {
        // Default: advance to next screen in cycle
        currentScreen   = (currentScreen + 1) % SCREEN_COUNT;
        lastDrawnSecond = -1;
        needsRedraw     = true;
      }
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
    lastPacketMillis   = 0;
    currentScreen      = SCREEN_CLOCK;
    lastDrawnSecond    = -1;
    needsRedraw        = true;
    lastActivityMillis = millis();  // give a full 15 s on clock before sleep
  }

  // Auto-sleep after 15 s of inactivity on the clock screen
  if (currentScreen == SCREEN_CLOCK && displayOn &&
      lastActivityMillis > 0 && millis() - lastActivityMillis >= AUTO_CLOCK_MS) {
    displayOn = false;
    ttgo->closeBL();
  }

  // Battery screen: partial data refresh every 5 s (no fillScreen = no flash)
  static unsigned long lastBattRefresh = 0;
  if (currentScreen == SCREEN_BATTERY && !needsRedraw &&
      millis() - lastBattRefresh >= 5000) {
    lastBattRefresh = millis();
    updateBatteryData();
  }

  // Stopwatch screen: partial time-only update every 100 ms (no full redraw = no flicker)
  static unsigned long lastSwRedraw = 0;
  if (currentScreen == SCREEN_STOPWATCH && swRunning &&
      millis() - lastSwRedraw >= 100) {
    lastSwRedraw = millis();
    updateStopwatchTime();
  }

  // Clock screen: redraw every second when synced, every 50 ms while spinning
  if (currentScreen == SCREEN_CLOCK && !needsRedraw) {
    if (baseEpoch == 0) {
      if (millis() - lastAnimMillis >= ANIM_FRAME_MS) {
        lastAnimMillis = millis();
        needsRedraw    = true;
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
