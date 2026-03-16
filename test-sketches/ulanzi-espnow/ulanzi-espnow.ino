/*
 * ESP-NOW Receiver + Clock Display (Ulanzi TC001)
 *
 * Receives DMRD Homebrew + POCSAG packets from the MMDVM hotspot via ESP-NOW.
 * Tries to join WiFi (same router as hotspot = same channel).
 * Syncs clock from POCSAG RIC 224 time-beacon ("YYYYMMDDHHMMSS<YYMMDDHHmmSS>").
 * ArduinoOTA + web status page always active when WiFi is up.
 *
 * Configure modes and display settings in config.h.
 *
 * File layout (all compiled as one unit by Arduino):
 *   ulanzi-espnow.ino  — this file: includes, ALL globals, setup(), loop()
 *   buttons.ino        — loopButtons()
 *   buzzer.ino         — buzzer engine + setupBuzzer()
 *   display.ino        — fonts, drawing helpers, loopDisplay/Brightness/AutoRotate
 *   receiver.ino       — ESP-NOW onReceive(), processPocsagPacket(), setupReceiver()
 *   sensor.ino         — DS1307 RTC + SHT31
 *   settings.ino       — loadSettings() / saveSettings()
 *   web.ino            — setupOTA() + setupWebServer() + all HTTP handlers
 *   filesystem.ino     — LittleFS init (setupFilesystem())
 */

#include "config.h"
#include <FastLED.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <time.h>
#include <Preferences.h>
#include <Wire.h>
#include <LittleFS.h>
#include <AnimatedGIF.h>
#include <JPEGDEC.h>
#include "web/main.h"
#include "web/settings.h"
#include "web/system.h"
#include "web/files.h"
#include "SHT31.h"

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

// ============================================================
// POCSAG display state
// ============================================================
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

// ============================================================
// Buzzer settings & non-blocking tone engine
// ============================================================
static bool    buzzerBootEnabled   = true;
static uint8_t buzzerBootVolume    = BUZZER_VOL_BOOT;
static bool    buzzerPocsagEnabled = true;
static uint8_t buzzerPocsagVolume  = BUZZER_VOL_POCSAG;
static bool    buzzerClickEnabled  = true;
static uint8_t buzzerClickVolume   = BUZZER_VOL_CLICK;

// Tone queue — written from ESP-NOW callback (Core 0), processed in loop() (Core 1)
static volatile uint16_t buzzerQFreq     = 0;
static volatile uint16_t buzzerQDuration = 0;
static volatile uint8_t  buzzerQDuty     = 0;
static volatile bool     buzzerQPending  = false;
static unsigned long     buzzerEndMs     = 0;

// ============================================================
// Auto-rotation
// ============================================================
static bool    autoRotateEnabled     = false;
static uint8_t autoRotateIntervalSec = 5;    // seconds per screen in rotation

// ============================================================
// SHT31 + display mode
// ============================================================
static SHT31        sht31Sensor;
static bool         sht31Available = false;
static float        sht31Temp      = 0.0f;
static float        sht31Hum       = 0.0f;

enum DisplayMode : uint8_t { MODE_CLOCK = 0, MODE_TEMP, MODE_HUMIDITY, MODE_BATTERY, MODE_COUNT };
static DisplayMode   displayMode     = MODE_CLOCK;
static unsigned long modeActiveUntil = 0;
#define MODE_TIMEOUT_MS  10000   // ms before auto-returning to clock (manual mode)

// Icon filenames (GIF from LittleFS; missing file = built-in bitmap fallback)
static char iconTempFile[32] = "/temp.gif";
static char iconHumFile[32]  = "/hum.gif";
static char iconBatFile[32]  = "/bat.gif";

// ============================================================
// Web status (updated by receive code, served via /api/status)
// ============================================================
static uint32_t  wsCountDmr    = 0;
static uint32_t  wsCountPocsag = 0;
struct WsPocsagEntry { uint32_t ric; char msg[POCSAG_MSG_MAX_LEN + 1]; };
static WsPocsagEntry wsPocsagLog[POCSAG_LOG_SIZE] = {};
static uint8_t       wsPocsagHead = 0;   // next write slot
static uint8_t       wsPocsagFill = 0;   // valid entries (0..POCSAG_LOG_SIZE)
static WebServer     webServer(80);
static QueueHandle_t pocsagRxQueue = nullptr;

// Shared state — set by sensor/receiver tasks, read by display and web handler
static bool          timeSynced    = false;  // true once clock is running (RTC or POCSAG)
static bool          pocsagSynced  = false;  // true only after POCSAG RIC 224 has confirmed time
static volatile bool otaInProgress = false;

// ============================================================
// RTC state
// ============================================================
static bool rtcAvailable = false;

// ============================================================
// Filesystem state
// ============================================================
static bool fsAvailable = false;

// ============================================================
// OTA state
// ============================================================
static bool otaStarted  = false;
static int  otaLastBarW = -1;   // reset each OTA session in onStart

// ============================================================
// IP address scroll — armed once after WiFi connects
// ============================================================
static bool          ipScrollActive = false;
static char          ipScrollMsg[32] = {};
static int           ipScrollLen     = 0;
static int           ipScrollX       = 0;
static int           ipScrollPass    = 0;
static unsigned long ipScrollLast    = 0;

// ============================================================
// DMR receive state
// ============================================================
#if RECV_DMR
static uint32_t rxTotalDmr   = 0;
static uint32_t callFrames   = 0;
static uint32_t callSrc      = 0;
static uint32_t callDst      = 0;
static uint8_t  callSlot     = 0;
static unsigned long callStart = 0;
#endif

// ============================================================
// POCSAG receive state
// ============================================================
#if RECV_POCSAG
static uint32_t rxTotalPocsag = 0;
#endif

// ============================================================
// Web + OTA task (Core 0)
// ============================================================
static void webTaskFn(void*) {
  for (;;) {
    if (otaStarted) ArduinoOTA.handle();
    webServer.handleClient();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
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
  drawBootScreen();
  // FastLED.clear();    // clear boot logo immediately; loop() will run scanner
  // FastLED.show();

  setupFilesystem();
  setupRTC();
  setupSHT31();   // probe 0x44; Wire already started by setupRTC()
  setupBuzzer();
  setupReceiver();

  // Arm IP scroll if WiFi connected (plays as first display in loop())
  if (WiFi.status() == WL_CONNECTED) {
    snprintf(ipScrollMsg, sizeof(ipScrollMsg), "IP:%s",
             WiFi.localIP().toString().c_str());
    ipScrollLen  = strlen(ipScrollMsg);
    ipScrollX    = MATRIX_WIDTH;
    ipScrollPass = 0;
    ipScrollActive = true;
    Serial.printf("[DISP] IP scroll: %s\n", ipScrollMsg);
  }

#if RECV_POCSAG
  pocsagRxQueue = xQueueCreate(4, sizeof(EspNowPocsagPacket));
#endif
  xTaskCreatePinnedToCore(webTaskFn, "webTask", 8192, nullptr, 1, nullptr, 0);
  Serial.println("[RTOS] webTask started on core 0");
}

void loop() {
  // Core 1: POCSAG queue drain, display, sensors, buttons

#if RECV_POCSAG
  EspNowPocsagPacket pkt;
  while (xQueueReceive(pocsagRxQueue, &pkt, 0) == pdTRUE)
    processPocsagPacket(pkt);
#endif

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
  loopBuzzer();
  loopSHT31();
  loopAutoRotate();
  loopDisplay();
}
