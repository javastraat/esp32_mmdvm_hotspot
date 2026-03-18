#pragma once
// globals.h — Shared types, structs, enums, and extern declarations.
// All variables declared extern here are DEFINED in ulanzi-espnow.ino.
// Every module .cpp includes this file.

#include "config.h"
#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "SHT31.h"

// ── Packet definitions (must match system/system_espnow.h exactly) ────────────

#define ESPNOW_TYPE_DMR_NET  0x10
#define ESPNOW_TYPE_POCSAG   0x11

#define POCSAG_MSG_MAX_LEN       80
#define FUNCTIONAL_NUMERIC        0
#define FUNCTIONAL_ALPHANUMERIC   3

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

struct WsPocsagEntry {
  uint32_t ric;
  char     msg[POCSAG_MSG_MAX_LEN + 1];
};

// ── LED matrix ────────────────────────────────────────────────────────────────

#define NUM_LEDS      256
#define MATRIX_WIDTH   32
#define MATRIX_HEIGHT   8

extern CRGB leds[NUM_LEDS];

// ── Display mode ──────────────────────────────────────────────────────────────

enum DisplayMode : uint8_t { MODE_CLOCK = 0, MODE_TEMP, MODE_HUMIDITY, MODE_BATTERY, MODE_COUNT };

#define MODE_TIMEOUT_MS  10000

extern DisplayMode   displayMode;
extern unsigned long modeActiveUntil;

// ── Icon filenames ────────────────────────────────────────────────────────────

extern char iconTempFile[32];
extern char iconHumFile[32];
extern char iconBatFile[32];
extern char iconPocsagFile[32];

// ── POCSAG display state ──────────────────────────────────────────────────────

#if RECV_POCSAG
extern char          pocsagMsg[POCSAG_MSG_MAX_LEN + 1];
extern int           pocsagMsgLen;
extern bool          pocsagMsgActive;
extern bool          pocsagIsScrolling;
extern int           pocsagScrollX;
extern int           pocsagScrollPass;
extern unsigned long pocsagScrollLast;
extern unsigned long pocsagStaticUntil;
extern unsigned long pocsagStaticLastDraw;
#endif

// ── Brightness ────────────────────────────────────────────────────────────────

extern bool    autoBrightnessEnabled;
extern uint8_t currentBrightness;

// ── Buzzer settings & tone queue ──────────────────────────────────────────────

extern bool    buzzerBootEnabled;
extern uint8_t buzzerBootVolume;
extern bool    buzzerPocsagEnabled;
extern uint8_t buzzerPocsagVolume;
extern bool    buzzerClickEnabled;
extern uint8_t buzzerClickVolume;

extern volatile uint16_t buzzerQFreq;
extern volatile uint16_t buzzerQDuration;
extern volatile uint8_t  buzzerQDuty;
extern volatile bool     buzzerQPending;
extern unsigned long     buzzerEndMs;

// ── Auto-rotation ─────────────────────────────────────────────────────────────

extern bool    autoRotateEnabled;
extern uint8_t autoRotateIntervalSec;

// ── Sensors (SHT31 + RTC) ─────────────────────────────────────────────────────

extern SHT31  sht31Sensor;
extern bool   sht31Available;
extern float  sht31Temp;
extern float  sht31Hum;
extern bool   rtcAvailable;

// ── Web / receive counters ────────────────────────────────────────────────────

extern uint32_t      wsCountDmr;
extern uint32_t      wsCountPocsag;
extern WsPocsagEntry wsPocsagLog[POCSAG_LOG_SIZE];
extern uint8_t       wsPocsagHead;
extern uint8_t       wsPocsagFill;

// ── Shared runtime state ──────────────────────────────────────────────────────

extern WebServer     webServer;
extern QueueHandle_t pocsagRxQueue;
extern bool          timeSynced;
extern bool          pocsagSynced;
extern volatile bool otaInProgress;
extern bool          fsAvailable;
extern bool          otaStarted;
extern int           otaLastBarW;

// ── IP address scroll ─────────────────────────────────────────────────────────

extern bool          ipScrollActive;
extern char          ipScrollMsg[32];
extern int           ipScrollLen;
extern int           ipScrollX;
extern int           ipScrollPass;
extern unsigned long ipScrollLast;

// ── Icon preview (Show button in settings) ────────────────────────────────────

extern bool          iconPreviewActive;
extern char          iconPreviewFile[32];
extern unsigned long iconPreviewUntil;

// ── Screensaver ───────────────────────────────────────────────────────────────

extern bool          screensaverEnabled;
extern uint16_t      screensaverTimeoutSec;
extern char          screensaverFile[64];
extern bool          screensaverActive;
extern unsigned long screensaverIdleStart;

// ── DMR receive state ─────────────────────────────────────────────────────────

#if RECV_DMR
extern uint32_t      rxTotalDmr;
extern uint32_t      callFrames;
extern uint32_t      callSrc;
extern uint32_t      callDst;
extern uint8_t       callSlot;
extern unsigned long callStart;
#endif

// ── POCSAG receive count ──────────────────────────────────────────────────────

#if RECV_POCSAG
extern uint32_t rxTotalPocsag;
#endif

// ── Display colors ────────────────────────────────────────────────────────────

extern CRGB    colorClock;
extern CRGB    colorPocsag;
// Temperature zones (lo < threshLo → Lo color, threshLo–threshHi → Mid, > threshHi → Hi)
extern float   tempThreshLo;
extern float   tempThreshHi;
extern CRGB    colorTempLo;
extern CRGB    colorTempMid;
extern CRGB    colorTempHi;
// Humidity zones
extern float   humThreshLo;
extern float   humThreshHi;
extern CRGB    colorHumLo;
extern CRGB    colorHumMid;
extern CRGB    colorHumHi;
// Battery zones (percent)
extern uint8_t batThreshLo;
extern uint8_t batThreshHi;
extern CRGB    colorBatLo;
extern CRGB    colorBatMid;
extern CRGB    colorBatHi;
