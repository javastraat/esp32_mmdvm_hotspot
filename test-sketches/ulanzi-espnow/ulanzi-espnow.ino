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
 * File layout:
 *   ulanzi-espnow.ino    — global variable definitions, setup(), loop()
 *   globals.h            — shared extern declarations (included by every module)
 *   buzzer.h/.cpp        — LEDC buzzer engine
 *   buttons.h/.cpp       — debounced button handler
 *   display.h/.cpp       — LED matrix fonts, drawing helpers, display loop
 *   filesystem.h/.cpp    — LittleFS initialisation
 *   nvs_settings.h/.cpp  — NVS Preferences load/save
 *   receiver.h/.cpp      — ESP-NOW receive callback + POCSAG processing
 *   sensor.h/.cpp        — DS1307 RTC + SHT31 temperature/humidity
 *   web_server.h/.cpp    — ArduinoOTA + WebServer routes + web task
 *   web/                 — web page HTML constants (edit these to change the UI)
 */

#include "globals.h"
#include "buzzer.h"
#include "buttons.h"
#include "display.h"
#include "filesystem.h"
#include "nvs_settings.h"
#include "receiver.h"
#include "sensor.h"
#include "web_server.h"

// ── Sanity checks ─────────────────────────────────────────────────────────────

#if RECV_DMR == false && RECV_POCSAG == false
  #error "Enable at least one of RECV_DMR or RECV_POCSAG in config.h."
#endif

// ── Global variable definitions ───────────────────────────────────────────────
// All extern declarations for these are in globals.h.

CRGB leds[NUM_LEDS];

// Display state
DisplayMode   displayMode     = MODE_CLOCK;
unsigned long modeActiveUntil = 0;

// Device name — shown on boot screen (max 8 chars, uppercase)
char bootName[9]  = "ULANZI";
// mDNS + ArduinoOTA hostname — device reachable as <name>.local (max 31 chars, lowercase)
char mdnsName[32] = MDNS_HOSTNAME;

// Icon filenames (GIF/JPEG from LittleFS) — empty = no icon, show text only
char iconTempFile[32]   = "";
char iconHumFile[32]    = "";
char iconBatFile[32]    = "";
char iconPocsagFile[32] = "";

// POCSAG display state
#if RECV_POCSAG
char  pocsagMsg[POCSAG_MSG_MAX_LEN + 1] = {};
int   pocsagMsgLen        = 0;
bool  pocsagMsgActive     = false;
bool  pocsagIsScrolling   = false;
int   pocsagScrollX       = 0;
int   pocsagScrollPass    = 0;
unsigned long pocsagScrollLast    = 0;
unsigned long pocsagStaticUntil   = 0;
unsigned long pocsagStaticLastDraw = 0;  // 0 = force immediate draw
#endif

// Brightness (LDR auto + manual)
bool    autoBrightnessEnabled = true;
uint8_t currentBrightness     = LED_BRIGHTNESS;

// Buzzer settings & non-blocking tone engine
// Tone queue — written from ESP-NOW callback (Core 0), processed in loop() (Core 1)
bool    buzzerBootEnabled   = true;
uint8_t buzzerBootVolume    = BUZZER_VOL_BOOT;
bool    buzzerPocsagEnabled = true;
uint8_t buzzerPocsagVolume  = BUZZER_VOL_POCSAG;
bool    buzzerClickEnabled  = true;
uint8_t buzzerClickVolume   = BUZZER_VOL_CLICK;
volatile uint16_t buzzerQFreq     = 0;
volatile uint16_t buzzerQDuration = 0;
volatile uint8_t  buzzerQDuty     = 0;
volatile bool     buzzerQPending  = false;
unsigned long     buzzerEndMs     = 0;

// Auto-rotation
bool    autoRotateEnabled     = false;
uint8_t autoRotateIntervalSec = 5;

// SHT31 + RTC
SHT31  sht31Sensor;
bool   sht31Available = false;
float  sht31Temp      = 0.0f;
float  sht31Hum       = 0.0f;
bool   rtcAvailable   = false;

// Web status (updated by receive code, served via /api/status)
uint32_t      wsCountDmr    = 0;
uint32_t      wsCountPocsag = 0;
WsPocsagEntry wsPocsagLog[POCSAG_LOG_SIZE] = {};
uint8_t       wsPocsagHead = 0;
uint8_t       wsPocsagFill = 0;
WebServer     webServer(80);
QueueHandle_t pocsagRxQueue  = nullptr;

// Shared runtime state
bool          timeSynced     = false;  // true once clock is running (RTC or POCSAG)
bool          pocsagSynced   = false;  // true only after POCSAG RIC 224 has confirmed time
volatile bool otaInProgress  = false;
bool          fsAvailable    = false;
bool          otaStarted      = false;
bool          mdnsStarted     = false;
bool          debugLogEnabled = false; // NVS: "debug_log" — enables DLOG() output
int           otaLastBarW    = -1;

// IP address scroll — armed once after WiFi connects
bool          ipScrollActive = false;
char          ipScrollMsg[32] = {};
int           ipScrollLen     = 0;
int           ipScrollX       = 0;
int           ipScrollPass    = 0;
unsigned long ipScrollLast    = 0;

// Icon preview (Show button in settings)
bool          iconPreviewActive = false;
char          iconPreviewFile[32] = "";
unsigned long iconPreviewUntil    = 0;

// ESP-NOW protocol mode
bool recvPocsagEnabled = true;

// Indicators (AWTRIX3-style right-edge status dots)
bool          indicatorsEnabled    = true;
unsigned long pocsagIndicatorUntil = 0;  // millis() until POCSAG indicator clears

// ESP-NOW / POCSAG RIC settings (runtime, overridable via web)
uint32_t timePocRic       = TIME_POCSAG_RIC;
uint32_t callsignRic      = CALLSIGN_RIC;
uint32_t excludedRics[EXCLUDED_RICS_MAX] = {};
uint8_t  excludedRicsCount = 0;

// Screensaver
bool          screensaverEnabled    = false;
uint16_t      screensaverTimeoutSec = 60;
char          screensaverFile[64]   = "";
bool          screensaverActive     = false;
unsigned long screensaverIdleStart  = 0;
int16_t       screensaverBrightness = 50;  // -2=Auto (LDR), 0/10/50/120/255=fixed (default Dim)

// DMR receive state
#if RECV_DMR
uint32_t      rxTotalDmr   = 0;
uint32_t      callFrames   = 0;
uint32_t      callSrc      = 0;
uint32_t      callDst      = 0;
uint8_t       callSlot     = 0;
unsigned long callStart    = 0;
#endif

// POCSAG receive count
#if RECV_POCSAG
uint32_t rxTotalPocsag = 0;
#endif

// Display colors
CRGB    colorClock   = CRGB(255, 255, 255);
CRGB    colorPocsag  = CRGB(255, 160,   0);
// Temperature zones
float   tempThreshLo = 16.0f;
float   tempThreshHi = 20.0f;
CRGB    colorTempLo  = CRGB(  0, 160, 255);
CRGB    colorTempMid = CRGB(  0, 200,  50);
CRGB    colorTempHi  = CRGB(255,  80,   0);
// Humidity zones
float   humThreshLo  = 30.0f;
float   humThreshHi  = 50.0f;
CRGB    colorHumLo   = CRGB(255, 160,   0);
CRGB    colorHumMid  = CRGB(  0, 200,  50);
CRGB    colorHumHi   = CRGB(  0, 160, 255);
// Battery zones
uint8_t batThreshLo  = 30;
uint8_t batThreshHi  = 60;
CRGB    colorBatLo   = CRGB(220,  40,   0);
CRGB    colorBatMid  = CRGB(220, 180,   0);
CRGB    colorBatHi   = CRGB(  0, 200,  50);

// ── Arduino entry points ──────────────────────────────────────────────────────

void setup() {
  pinMode(15,         INPUT_PULLDOWN); // buzzer — stops high-pitch noise at boot
  pinMode(BTN_LEFT,   INPUT_PULLUP);
  pinMode(BTN_MIDDLE, INPUT_PULLUP);
  pinMode(BTN_RIGHT,  INPUT_PULLUP);
  pinMode(BAT_PIN,    INPUT);          // battery ADC

  Serial.begin(115200);
  delay(3000);
  LOG("\n\n=== ULANZI ESP-NOW Monitor + Clock ===\n");

  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  setupDisplay();
  setupFilesystem();
  loadSettings();                     // load NVS before boot screen so bootName/brightness are applied
  FastLED.setBrightness(currentBrightness);  // apply saved brightness before first display tick
  drawBootScreen();
  setupRTC();
  setupSHT31();   // probe 0x44; Wire already started by setupRTC()
  setupBuzzer();
  setupReceiver();// connects WiFi → calls setupOTA() → starts webServer

  // Arm IP scroll if WiFi connected (plays as first display in loop())
  if (WiFi.status() == WL_CONNECTED) {
    snprintf(ipScrollMsg, sizeof(ipScrollMsg), "IP:%s",
             WiFi.localIP().toString().c_str());
    ipScrollLen  = strlen(ipScrollMsg);
    ipScrollX    = MATRIX_WIDTH;
    ipScrollPass = 0;
    ipScrollActive = true;
    LOG("[DISP] IP scroll: %s\n", ipScrollMsg);
  }

#if RECV_POCSAG
  pocsagRxQueue = xQueueCreate(4, sizeof(EspNowPocsagPacket));
#endif

  screensaverIdleStart = millis();  // start idle countdown from boot
  initWebTask();   // starts webTask  on Core 0 (ArduinoOTA.handle + webServer.handleClient)
  initMqttTask();  // starts mqttTask on Core 0 (connects after WiFi settles)
  LOG("[RTOS] webTask + mqttTask started on core 0\n");
}

void loop() {
  // Core 1: drain POCSAG queue, update display, sensors, buttons

#if RECV_POCSAG
  EspNowPocsagPacket pkt;
  while (xQueueReceive(pocsagRxQueue, &pkt, 0) == pdTRUE)
    processPocsagPacket(pkt);
#endif

#if ESPNOW_DEBUG
  static unsigned long lastHb = 0;
  if (millis() - lastHb >= 5000) {
    lastHb = millis();
    LOG("[RX] alive %lus | DMR:%lu POCSAG:%lu\n",
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
    LOG("[RX-DMR] ... src=%-8lu  frames=%lu  dur=%lus\n", callSrc, callFrames, dur);
  }
#endif

  loopButtons();
  loopBrightness();
  loopBuzzer();
  loopSHT31();
  loopAutoRotate();
  loopDisplay();
}
