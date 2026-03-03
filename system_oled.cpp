// System message/reboot display state
volatile bool rebootPending = false;
volatile bool firmwareReadyMessageActive = false;
/*
 * OLED Display System Module Implementation
 */

#include "system/system_oled.h"
#include "system/system_logger.h"
#include "system/system_webserver.h"
#include "system/system_eth.h"
#include "system/system_wifi.h"
#include "system/system_ntp.h"
#include "system/service_mqtt.h"
#include "system/system_arduinoota.h"
#include "include/sdcard_handlers.h"
#include "mmdvm/mmdvm_dmr.h"
#include "mmdvm/mmdvm_dapnet.h"
#include <WiFi.h>
#include <LittleFS.h>

// External runtime settings from .ino
extern String userCallsign;
extern bool modeDmrEnabled, modeDstarEnabled, modeYsfEnabled;
extern bool modeP25Enabled, modeNxdnEnabled, modePocsagEnabled;
extern bool mqttEnabled;
extern bool ethEnabled;
extern int requestCount;
extern String mqttOledTaskTopic; // MQTT topic for OLED task status updates
extern bool dapnetEnabled;
extern bool telegramEnabled;

TaskHandle_t oledTaskHandle = NULL;

// Create display object
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// OLED display state
volatile bool oledDisplayOn = true;
volatile bool buttonPressed = false;
unsigned long lastButtonPress = 0;
const unsigned long BUTTON_DEBOUNCE = 200; // 200ms debounce

// DMR TX user info display state
volatile bool dmrTxUserActive = false;
String dmrTxUserCallsign = "";
String dmrTxUserId = "";
String dmrTxUserName = "";
String dmrTxUserCity = "";
// String dmrTxUserState = ""; // Not used in display, reserved for future
String dmrTxUserCountry = "";
unsigned long dmrTxUserDisplayUntil = 0;
const unsigned long DMR_TX_USER_DISPLAY_TIMEOUT = 2000; // ms after TX end

// DMR call history (Net→RF, last 15 calls)
#define DMR_CALL_HISTORY_SIZE 15
struct DmrCallEntry {
  String callsign;
  String id;
  String name;
  String city;
  String country;
  String time;       // "HH:MM:SS"
  uint32_t durationSec = 0; // call length in seconds (backfilled on end)
};
static DmrCallEntry dmrCallHistory[DMR_CALL_HISTORY_SIZE];
static int dmrCallHistoryCount = 0;
static int dmrCallHistoryHead = 0; // next write position
static unsigned long dmrCallStartMillis = 0; // millis() when current call started

void setDmrTxUserInfo(const String& callsign, const String& id, const String& name, const String& city, const String& state, const String& country) {
  // Detect new call: callsign changed or no call was active
  if (callsign.length() > 0 && (callsign != dmrTxUserCallsign || !dmrTxUserActive)) {
    // Backfill duration into the previous history entry
    if (dmrTxUserActive && dmrCallHistoryCount > 0) {
      int prevIdx = (dmrCallHistoryHead - 1 + DMR_CALL_HISTORY_SIZE) % DMR_CALL_HISTORY_SIZE;
      unsigned long elapsed = millis() - dmrCallStartMillis;
      dmrCallHistory[prevIdx].durationSec = elapsed / 1000;
    }
    char timeBuf[9];
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    dmrCallHistory[dmrCallHistoryHead] = {callsign, id, name, city, country, String(timeBuf), 0};
    dmrCallHistoryHead = (dmrCallHistoryHead + 1) % DMR_CALL_HISTORY_SIZE;
    if (dmrCallHistoryCount < DMR_CALL_HISTORY_SIZE) dmrCallHistoryCount++;
    dmrCallStartMillis = millis();
  }
  dmrTxUserCallsign = callsign;
  dmrTxUserId = id;
  dmrTxUserName = name;
  dmrTxUserCity = city;
  // dmrTxUserState = state; // Not used in display, reserved for future
  dmrTxUserCountry = country;
  dmrTxUserActive = true;
  // Always refresh the timeout on every frame, so display stays as long as TX is active
  dmrTxUserDisplayUntil = millis() + DMR_TX_USER_DISPLAY_TIMEOUT;
}

// Helper: escape JSON string value
static String dmrJsonEscape(const String& s) {
  String out;
  out.reserve(s.length());
  for (int i = 0; i < (int)s.length(); i++) {
    char c = s[i];
    if (c == '"')       out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else                out += c;
  }
  return out;
}

// Returns JSON with current call state + last 15 call history
String getDmrActivityJson() {
  // Auto-clear if TX has been idle longer than the display timeout.
  // Normally the OLED task does this, but if OLED is disabled we still
  // need to reset the active flag so the web On Air badge goes back to IDLE.
  if (dmrTxUserActive && dmrTxUserDisplayUntil > 0 && millis() > dmrTxUserDisplayUntil) {
    clearDmrTxUserInfo();
  }
  String json = "{";
  json += "\"active\":" + String(dmrTxUserActive ? "true" : "false");
  json += ",\"elapsed\":" + String(dmrTxUserActive ? (millis() - dmrCallStartMillis) / 1000UL : 0UL);
  json += ",\"current\":{";
  json += "\"callsign\":\"" + dmrJsonEscape(dmrTxUserCallsign) + "\"";
  json += ",\"id\":\"" + dmrJsonEscape(dmrTxUserId) + "\"";
  json += ",\"name\":\"" + dmrJsonEscape(dmrTxUserName) + "\"";
  json += ",\"city\":\"" + dmrJsonEscape(dmrTxUserCity) + "\"";
  json += ",\"country\":\"" + dmrJsonEscape(dmrTxUserCountry) + "\"";
  json += "},\"history\":[";
  for (int i = 0; i < dmrCallHistoryCount; i++) {
    int idx = (dmrCallHistoryHead - 1 - i + DMR_CALL_HISTORY_SIZE) % DMR_CALL_HISTORY_SIZE;
    if (i > 0) json += ",";
    json += "{\"callsign\":\"" + dmrJsonEscape(dmrCallHistory[idx].callsign) + "\"";
    json += ",\"id\":\"" + dmrJsonEscape(dmrCallHistory[idx].id) + "\"";
    json += ",\"name\":\"" + dmrJsonEscape(dmrCallHistory[idx].name) + "\"";
    json += ",\"city\":\"" + dmrJsonEscape(dmrCallHistory[idx].city) + "\"";
    json += ",\"country\":\"" + dmrJsonEscape(dmrCallHistory[idx].country) + "\"";
    json += ",\"time\":\"" + dmrCallHistory[idx].time + "\"";
    json += ",\"duration\":" + String(dmrCallHistory[idx].durationSec) + "}";
  }
  json += "]}";
  return json;
}

void clearDmrTxUserInfo() {
  // Backfill duration into the last history entry (subtract the 2s display linger timeout)
  if (dmrCallHistoryCount > 0) {
    int prevIdx = (dmrCallHistoryHead - 1 + DMR_CALL_HISTORY_SIZE) % DMR_CALL_HISTORY_SIZE;
    unsigned long elapsed = millis() - dmrCallStartMillis;
    uint32_t dur = (elapsed >= DMR_TX_USER_DISPLAY_TIMEOUT)
                   ? (elapsed - DMR_TX_USER_DISPLAY_TIMEOUT) / 1000
                   : 0;
    dmrCallHistory[prevIdx].durationSec = dur;
  }
  dmrTxUserActive = false;
  dmrTxUserCallsign = "";
  dmrTxUserId = "";
  dmrTxUserName = "";
  dmrTxUserCity = "";
  // dmrTxUserState = "";
  dmrTxUserCountry = "";
  dmrTxUserDisplayUntil = 0;
}

void drawDmrTxUserInfo() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Callsign extra big, centered, near top
  // Fall back to DMR ID at size 2 while async lookup is still pending
  int16_t x1, y1; uint16_t w, h;
  if (dmrTxUserCallsign.length() > 0) {
    display.setTextSize(3);
    display.getTextBounds(dmrTxUserCallsign, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((OLED_WIDTH - w) / 2, 2);
    display.print(dmrTxUserCallsign);
  } else {
    display.setTextSize(2);
    display.getTextBounds(dmrTxUserId, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((OLED_WIDTH - w) / 2, 8);
    display.print(dmrTxUserId);
  }

  // Name, big, centered, moved down 5 pixels
  display.setTextSize(2);
  String nameOnly = dmrTxUserName;
  display.getTextBounds(nameOnly, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((OLED_WIDTH - w) / 2, 27);
  display.print(nameOnly);

  // City, small, centered, moved down 5 pixels
  display.setTextSize(1);
  String cityOnly = dmrTxUserCity;
  display.getTextBounds(cityOnly, 0, 0, &x1, &y1, &w, &h);
  int cityY = 45;
  if (cityOnly.length() > 0) {
    display.setCursor((OLED_WIDTH - w) / 2, cityY);
    display.print(cityOnly);
  }

  // Country, small, centered, bottom row
  String countryOnly = dmrTxUserCountry;
  display.getTextBounds(countryOnly, 0, 0, &x1, &y1, &w, &h);
  int countryY = 56;
  if (countryOnly.length() > 0) {
    display.setCursor((OLED_WIDTH - w) / 2, countryY);
    display.print(countryOnly);
  }

  display.display();
}

// POCSAG TX info display state
volatile bool pocsagTxDisplayActive = false;
String pocsagTxRic = "";
String pocsagTxMessage = "";
unsigned long pocsagTxDisplayUntil = 0;
const unsigned long POCSAG_TX_DISPLAY_TIMEOUT = 5000; // ms

// POCSAG TX history (last 15 transmitted messages)
#define POCSAG_TX_HISTORY_SIZE 15
struct PocsagTxEntry {
  String ric;
  String message;
  String time; // "HH:MM:SS"
};
static PocsagTxEntry pocsagTxHistory[POCSAG_TX_HISTORY_SIZE];
static int pocsagTxHistoryCount = 0;
static int pocsagTxHistoryHead  = 0; // next write slot

void setPocsagTxInfo(const String& ric, const String& message) {
  // Record in TX history
  char timeBuf[9];
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
  pocsagTxHistory[pocsagTxHistoryHead] = {ric, message, String(timeBuf)};
  pocsagTxHistoryHead = (pocsagTxHistoryHead + 1) % POCSAG_TX_HISTORY_SIZE;
  if (pocsagTxHistoryCount < POCSAG_TX_HISTORY_SIZE) pocsagTxHistoryCount++;

  pocsagTxRic = ric;
  pocsagTxMessage = message;
  pocsagTxDisplayActive = true;
  pocsagTxDisplayUntil = millis() + POCSAG_TX_DISPLAY_TIMEOUT;
}

String getPocsagTxHistoryJson() {
  String json = "{\"count\":" + String(pocsagTxHistoryCount) + ",\"items\":[";
  for (int i = 0; i < pocsagTxHistoryCount; i++) {
    // newest first
    int idx = (pocsagTxHistoryHead - 1 - i + POCSAG_TX_HISTORY_SIZE) % POCSAG_TX_HISTORY_SIZE;
    if (i > 0) json += ",";
    String escapedMsg = pocsagTxHistory[idx].message;
    escapedMsg.replace("\\", "\\\\");
    escapedMsg.replace("\"", "\\\"");
    json += "{\"ric\":\"" + pocsagTxHistory[idx].ric + "\"";
    json += ",\"msg\":\"" + escapedMsg + "\"";
    json += ",\"time\":\"" + pocsagTxHistory[idx].time + "\"}";
  }
  json += "]}";
  return json;
}

void drawPocsagTxInfo() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int16_t x1, y1; uint16_t w, h;

  // Row 1: "POCSAG TX" centered
  String header = "POCSAG TX";
  display.getTextBounds(header, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((OLED_WIDTH - w) / 2, 0);
  display.print(header);

  display.drawLine(0, 10, OLED_WIDTH - 1, 10, SSD1306_WHITE);

  // Middle: Message text, size 2, up to 2 lines of 10 chars each, centered
  display.setTextSize(2);
  String msg = pocsagTxMessage;
  String line1 = msg.substring(0, 10);
  display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((OLED_WIDTH - w) / 2, 14);
  display.print(line1);
  if (msg.length() > 10) {
    String line2 = msg.substring(10, 20);
    display.getTextBounds(line2, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((OLED_WIDTH - w) / 2, 34);
    display.print(line2);
  }
  display.setTextSize(1);

  // Divider above RIC
  display.drawLine(0, 54, OLED_WIDTH - 1, 54, SSD1306_WHITE);

  // Bottom row: RIC centered
  String ricLabel = "RIC: " + pocsagTxRic;
  display.getTextBounds(ricLabel, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((OLED_WIDTH - w) / 2, 56);
  display.print(ricLabel);

  display.display();
}

// Bottom bar cycling state
int oledBottomCycle = 0;
unsigned long lastBottomCycle = 0;
const unsigned long BOTTOM_CYCLE_INTERVAL = 5000; // 5 seconds

// ISR for button press
void IRAM_ATTR buttonISR()
{
  unsigned long currentTime = millis();
  if (currentTime - lastButtonPress > BUTTON_DEBOUNCE)
  {
    buttonPressed = true;
    lastButtonPress = currentTime;
  }
}

// Boot logo definition
#define LOGO_WIDTH 128
#define LOGO_HEIGHT 64
static const unsigned char PROGMEM logo_bmp[] = {
    // 'p', 128x64px
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1f, 0xfc, 0x1e, 0x9f, 0xf0, 0x3c, 0x3c, 0x03, 0xf3, 0xf0, 0x00, 0x40, 0x00, 0x00, 0x00, 0x20,
    0x07, 0x0c, 0x31, 0x87, 0x38, 0x7e, 0x7e, 0x00, 0xe1, 0xc0, 0x00, 0x40, 0x00, 0x00, 0x00, 0x20,
    0x07, 0x04, 0x70, 0x87, 0x1c, 0x8e, 0x4f, 0x00, 0xe1, 0xc0, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x60,
    0x07, 0x20, 0x70, 0x87, 0x1c, 0x0c, 0x07, 0x00, 0xe1, 0xc0, 0xf3, 0xf3, 0xbe, 0xc0, 0x79, 0xf8,
    0x07, 0x20, 0x78, 0x07, 0x1c, 0x18, 0x07, 0x00, 0xe1, 0xc1, 0x99, 0xc6, 0x6f, 0x60, 0xcc, 0xe0,
    0x07, 0xe0, 0x3e, 0x07, 0x38, 0x3c, 0x06, 0x00, 0xff, 0xc3, 0x9d, 0xc7, 0x2e, 0x71, 0xce, 0xe0,
    0x07, 0x20, 0x0f, 0x87, 0xf0, 0x1e, 0x0c, 0x00, 0xe1, 0xc3, 0x9d, 0xc7, 0x8e, 0x71, 0xce, 0xe0,
    0x07, 0x20, 0x43, 0xc7, 0x00, 0x0f, 0x08, 0x00, 0xe1, 0xc3, 0x9d, 0xc3, 0xce, 0x71, 0xce, 0xe0,
    0x07, 0x02, 0x41, 0xc7, 0x00, 0x07, 0x10, 0x00, 0xe1, 0xc3, 0x9d, 0xc1, 0xee, 0x71, 0xce, 0xe0,
    0x07, 0x02, 0x61, 0xc7, 0x00, 0xc7, 0x20, 0x80, 0xe1, 0xc3, 0x9d, 0xc4, 0xee, 0x71, 0xce, 0xe0,
    0x07, 0x06, 0x71, 0x87, 0x00, 0xe6, 0x7f, 0x00, 0xe1, 0xc1, 0x99, 0xd6, 0x6f, 0x60, 0xcc, 0xe8,
    0x1f, 0xfe, 0x4f, 0x1f, 0xc0, 0x78, 0xff, 0x03, 0xf3, 0xf0, 0xf0, 0xe5, 0xce, 0xc0, 0x78, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x38, 0x00, 0x1f, 0xbf, 0x0e, 0x7f, 0x70, 0xf1, 0xd0, 0x1c, 0x07, 0xef, 0xc3, 0x87, 0x8f, 0x00,
    0x18, 0x00, 0x0c, 0xd9, 0x8f, 0x31, 0x38, 0xe3, 0x30, 0x32, 0x03, 0x36, 0x66, 0xc3, 0x19, 0x80,
    0x18, 0x00, 0x0c, 0xd8, 0xd3, 0x30, 0x38, 0xe6, 0x10, 0x32, 0x03, 0x36, 0x36, 0xc3, 0x30, 0xc0,
    0x1e, 0x3b, 0x0c, 0xd8, 0xc3, 0x34, 0x3d, 0x66, 0x00, 0x34, 0x03, 0x36, 0x36, 0xc3, 0x30, 0xc0,
    0x1b, 0x1a, 0x0f, 0x98, 0xc2, 0x3c, 0x2d, 0x66, 0x00, 0x5b, 0x83, 0xe6, 0x33, 0x83, 0x30, 0xc0,
    0x1b, 0x1a, 0x0c, 0x18, 0xc4, 0x34, 0x2d, 0x66, 0x00, 0xd9, 0x03, 0x06, 0x36, 0xc3, 0x30, 0xc0,
    0x1b, 0x0c, 0x0c, 0x18, 0xc8, 0x30, 0x2e, 0x66, 0x00, 0xce, 0x03, 0x06, 0x36, 0xdb, 0x30, 0xc0,
    0x1b, 0x0c, 0x0c, 0x19, 0x8f, 0x31, 0x26, 0x63, 0x10, 0xe6, 0x83, 0x06, 0x66, 0xdb, 0x19, 0x80,
    0x16, 0x04, 0x1e, 0x3f, 0x1f, 0x7f, 0x76, 0xf1, 0xe0, 0x7b, 0x07, 0x8f, 0xc3, 0x8e, 0x0f, 0x00,
    0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Icon bitmaps for OLED display (8x8 pixels)
#define ICON_WIDTH 8
#define ICON_HEIGHT 8

// WiFi icon (8x8)
static const unsigned char PROGMEM icon_wifi[] = {
    0b00000000,
    0b00111100,
    0b01000010,
    0b10011001,
    0b00100100,
    0b00011000,
    0b00011000,
    0b00000000};

// Ethernet/Cable icon (8x8)
static const unsigned char PROGMEM icon_ethernet[] = {
    0b00000000,
    0b01111110,
    0b01000010,
    0b01011010,
    0b01011010,
    0b01000010,
    0b01111110,
    0b00000000};

// Antenna/RF icon (8x8) - looks like radio waves
static const unsigned char PROGMEM icon_antenna[] = {
    0b00011000,
    0b00111100,
    0b01011010,
    0b10011001,
    0b00011000,
    0b00100100,
    0b01000010,
    0b10000001};


/*
 * Helper: Draw uptime + heap + requests on the middle section
 * Called from displaySystemInfo() to avoid repeating this in every mode block
 */
void drawUptimeAndHeap()
{
  // Uptime at y=26
  unsigned long uptimeSec = millis() / 1000;
  unsigned long hrs = uptimeSec / 3600;
  unsigned long mins = (uptimeSec % 3600) / 60;
  unsigned long secs = uptimeSec % 60;
  display.setCursor(0, 26);
  display.print("Up: ");
  if (hrs > 0)
  {
    display.print(hrs);
    display.print("h ");
  }
  display.print(mins);
  display.print("m ");
  display.print(secs);
  display.println("s");

  // Free heap + request count at y=38
  display.setCursor(0, 38);
  display.print("Heap:");
  display.print(ESP.getFreeHeap() / 1024);
  display.print("KB Req:");
  display.println(requestCount);
}

void drawDMRmiddle()
{
  // Uptime at y=26
  unsigned long uptimeSec = millis() / 1000;
  unsigned long hrs = uptimeSec / 3600;
  unsigned long mins = (uptimeSec % 3600) / 60;
  unsigned long secs = uptimeSec % 60;
  display.setCursor(0, 26);
  display.print("Up: ");
  if (hrs > 0)
  {
    display.print(hrs);
    display.print("h ");
  }
  display.print(mins);
  display.print("m ");
  display.print(secs);
  display.println("s");

  // Free heap + request count at y=38
  display.setCursor(0, 38);
  display.print("Heap:");
  display.print(ESP.getFreeHeap() / 1024);
  display.print("KB Req:");
  display.println(requestCount);
}

void drawDSTARmiddle()
{
  // Uptime at y=26
  unsigned long uptimeSec = millis() / 1000;
  unsigned long hrs = uptimeSec / 3600;
  unsigned long mins = (uptimeSec % 3600) / 60;
  unsigned long secs = uptimeSec % 60;
  display.setCursor(0, 26);
  display.print("Up: ");
  if (hrs > 0)
  {
    display.print(hrs);
    display.print("h ");
  }
  display.print(mins);
  display.print("m ");
  display.print(secs);
  display.println("s");

  // Free heap + request count at y=38
  display.setCursor(0, 38);
  display.print("Heap:");
  display.print(ESP.getFreeHeap() / 1024);
  display.print("KB Req:");
  display.println(requestCount);
}

void drawYSFmiddle()
{
  // Uptime at y=26
  unsigned long uptimeSec = millis() / 1000;
  unsigned long hrs = uptimeSec / 3600;
  unsigned long mins = (uptimeSec % 3600) / 60;
  unsigned long secs = uptimeSec % 60;
  display.setCursor(0, 26);
  display.print("Up: ");
  if (hrs > 0)
  {
    display.print(hrs);
    display.print("h ");
  }
  display.print(mins);
  display.print("m ");
  display.print(secs);
  display.println("s");

  // Free heap + request count at y=38
  display.setCursor(0, 38);
  display.print("Heap:");
  display.print(ESP.getFreeHeap() / 1024);
  display.print("KB Req:");
  display.println(requestCount);
}

void drawP25middle()
{
  // Uptime at y=26
  unsigned long uptimeSec = millis() / 1000;
  unsigned long hrs = uptimeSec / 3600;
  unsigned long mins = (uptimeSec % 3600) / 60;
  unsigned long secs = uptimeSec % 60;
  display.setCursor(0, 26);
  display.print("Up: ");
  if (hrs > 0)
  {
    display.print(hrs);
    display.print("h ");
  }
  display.print(mins);
  display.print("m ");
  display.print(secs);
  display.println("s");

  // Free heap + request count at y=38
  display.setCursor(0, 38);
  display.print("Heap:");
  display.print(ESP.getFreeHeap() / 1024);
  display.print("KB Req:");
  display.println(requestCount);
}

void drawNXDNmiddle()
{
  // Uptime at y=26
  unsigned long uptimeSec = millis() / 1000;
  unsigned long hrs = uptimeSec / 3600;
  unsigned long mins = (uptimeSec % 3600) / 60;
  unsigned long secs = uptimeSec % 60;
  display.setCursor(0, 26);
  display.print("Up: ");
  if (hrs > 0)
  {
    display.print(hrs);
    display.print("h ");
  }
  display.print(mins);
  display.print("m ");
  display.print(secs);
  display.println("s");

  // Free heap + request count at y=38
  display.setCursor(0, 38);
  display.print("Heap:");
  display.print(ESP.getFreeHeap() / 1024);
  display.print("KB Req:");
  display.println(requestCount);
}

void drawPOCSAGmiddle()
{
  // Uptime at y=26
  unsigned long uptimeSec = millis() / 1000;
  unsigned long hrs = uptimeSec / 3600;
  unsigned long mins = (uptimeSec % 3600) / 60;
  unsigned long secs = uptimeSec % 60;
  display.setCursor(0, 26);
  display.print("Up: ");
  if (hrs > 0)
  {
    display.print(hrs);
    display.print("h ");
  }
  display.print(mins);
  display.print("m ");
  display.print(secs);
  display.println("s");

  // Free heap + request count at y=38
  display.setCursor(0, 38);
  display.print("Heap:");
  display.print(ESP.getFreeHeap() / 1024);
  display.print("KB Req:");
  display.println(requestCount);
}

void drawDAPNETmiddle()
{
  // Uptime at y=26
  unsigned long uptimeSec = millis() / 1000;
  unsigned long hrs = uptimeSec / 3600;
  unsigned long mins = (uptimeSec % 3600) / 60;
  unsigned long secs = uptimeSec % 60;
  display.setCursor(0, 26);
  display.print("Up: ");
  if (hrs > 0)
  {
    display.print(hrs);
    display.print("h ");
  }
  display.print(mins);
  display.print("m ");
  display.print(secs);
  display.println("s");

  // Free heap + request count at y=38
  display.setCursor(0, 38);
  display.print("Heap:");
  display.print(ESP.getFreeHeap() / 1024);
  display.print("KB Req:");
  display.println(requestCount);
}

void initOledTask()
{
  BaseType_t result = xTaskCreatePinnedToCore(
      oledTask,
      "OLED Task",
      OLED_TASK_STACK,
      NULL,
      OLED_TASK_PRIORITY,
      &oledTaskHandle,
      0 // Run on core 0 (less critical tasks)
  );
  if (result != pdPASS)
    log_e("[OLED] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());
}

/*
 * TASK: OLED Display Update
 * Priority: Low - Visual feedback, not critical
 *
 * Updates OLED display with system information
 * Shows WiFi status, IP address, stats, etc.
 */
void oledTask(void *parameter)
{
  addLogMessage("[OLED Task] Started");
  publishMqtt(mqttOledTaskTopic.c_str(), "{\"event\":\"started\"}");

  // Initialize button with pullup
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
  addLogMessage("[OLED Task] Button interrupt attached to pin " + String(BUTTON_PIN));
  publishMqtt(mqttOledTaskTopic.c_str(), "{\"event\":\"button_init\",\"pin\":" + String(BUTTON_PIN) + "}");

  // Initialize I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS))
  {
    addLogMessage("[OLED Task] SSD1306 allocation failed!");
    publishMqtt(mqttOledTaskTopic.c_str(), "{\"event\":\"init_failed\"}");
    vTaskDelete(NULL); // Delete this task if display fails
    return;
  }

  addLogMessage("[OLED Task] Display initialized");
  publishMqtt(mqttOledTaskTopic.c_str(), "{\"event\":\"init_ok\"}");

  // Display boot logo — use custom /bootlogo.bin from LittleFS if present (1024 bytes, 128×64 1bpp)
  display.clearDisplay();
  static uint8_t customLogo[LOGO_WIDTH * LOGO_HEIGHT / 8]; // 1024 bytes on stack
  bool usedCustom = false;
  File lf = LittleFS.open("/bootlogo.bin", "r");
  if (lf && lf.size() == sizeof(customLogo)) {
    lf.read(customLogo, sizeof(customLogo));
    lf.close();
    display.drawBitmap(0, 0, customLogo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
    usedCustom = true;
    addLogMessage("[OLED Task] Custom boot logo loaded from LittleFS");
  } else {
    if (lf) lf.close();
    display.drawBitmap(0, 0, logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
  }
  display.display();
  addLogMessage("[OLED Task] Boot logo displayed");
  publishMqtt(mqttOledTaskTopic.c_str(), usedCustom ? "{\"event\":\"boot_logo\",\"custom\":true}" : "{\"event\":\"boot_logo\",\"custom\":false}");

  // Show logo for 5 seconds
  vTaskDelay(5000 / portTICK_PERIOD_MS);

  // Clear display
  // display.clearDisplay();
  // display.setTextSize(1);
  // display.setTextColor(SSD1306_WHITE);
  // display.setCursor(0, 0);
  // display.println("ESP32 RTOS");
  // display.println("Initializing...");
  // display.display();

  // Wait for any network connection (WiFi OR Ethernet)
  while (WiFi.status() != WL_CONNECTED && !ethConnected)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  addLogMessage("[OLED Task] Network available - starting display");
  //publishMqtt(mqttOledTaskTopic.c_str(), "{\"event\":\"start_display\"}");

  while (true)
  {
    // Check for button press
    if (buttonPressed)
    {
      buttonPressed = false;
      oledDisplayOn = !oledDisplayOn;

      if (oledDisplayOn)
      {
        addLogMessage("[OLED Task] Display turned ON");
        publishMqtt(mqttOledTaskTopic.c_str(), "{\"event\":\"display_on\"}");
        display.ssd1306_command(SSD1306_DISPLAYON);
      }
      else
      {
        addLogMessage("[OLED Task] Display turned OFF");
        publishMqtt(mqttOledTaskTopic.c_str(), "{\"event\":\"display_off\"}");
        display.ssd1306_command(SSD1306_DISPLAYOFF);
      }
    }

    // Always update framebuffer (web virtual OLED reads it even when physical display is off)
    {
      // Priority: ArduinoOTA > Firmware updates > File downloads > Reboot/message > DMR TX user info > Firmware ready > System info
      if (arduinoOtaUpdateActive) {
        displayArduinoOtaProgress();
      }
      else if (modemFirmwareUpdateActive || espFirmwareUpdateActive) {
        displayFirmwareUpdateProgress();
      }
      else if (csvDownloadActive || sqliteDownloadActive) {
        displayDownloadProgress();
      }
      else if (rebootPending) {
        displayRebootMessage();
      }
      else if (firmwareReadyMessageActive) {
        displayFirmwareReadyMessage();
      }
      else if (pocsagTxDisplayActive) {
        drawPocsagTxInfo();
        if (pocsagTxDisplayUntil > 0 && millis() > pocsagTxDisplayUntil) {
          pocsagTxDisplayActive = false;
        }
      }
      else if (dmrTxUserActive) {
        drawDmrTxUserInfo();
        // Auto-clear after timeout
        if (dmrTxUserDisplayUntil > 0 && millis() > dmrTxUserDisplayUntil) {
          clearDmrTxUserInfo();
        }
      }
      else {
        displaySystemInfo();
      }
    }

    vTaskDelay(OLED_UPDATE_INTERVAL / portTICK_PERIOD_MS);
  }
}

void displaySystemInfo()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // ===== TOP BAR: Date (left), Time (center), Icons (right) =====
  String dateStr = "--/--";
  String timeStr = "--:--";

  struct tm timeinfo;
  if (isTimeSynced() && getLocalTime(&timeinfo))
  {
    char dateBuf[8];
    char timeBuf[8];
    strftime(dateBuf, sizeof(dateBuf), "%d/%m", &timeinfo);
    strftime(timeBuf, sizeof(timeBuf), "%H:%M", &timeinfo);
    dateStr = String(dateBuf);
    timeStr = String(timeBuf);
  }

  // Date on the left
  display.setCursor(0, 0);
  display.print(dateStr);

  // Time centered
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((OLED_WIDTH - w) / 2, 0);
  display.print(timeStr);

  // Icons on the right (right to left): WiFi, ETH, Antenna, DMR, Pager
  int iconX = OLED_WIDTH - ICON_WIDTH;

  // WiFi icon (rightmost)
  if (WiFi.status() == WL_CONNECTED)
  {
    display.drawBitmap(iconX, 0, icon_wifi, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
    iconX -= (ICON_WIDTH + 2);
  }

  // Ethernet icon
  if (ethConnected)
  {
    display.drawBitmap(iconX, 0, icon_ethernet, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
    iconX -= (ICON_WIDTH + 2);
  }

  // // Antenna icon (if any MMDVM mode is enabled)
  // bool anyModeOn = modeDmrEnabled || modeDstarEnabled || modeYsfEnabled || modeP25Enabled || modeNxdnEnabled || modePocsagEnabled;
  // if (anyModeOn)
  // {
  //   display.drawBitmap(iconX, 0, icon_antenna, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
  //   iconX -= (ICON_WIDTH + 2);
  // }
  // 'M' = MQTT connected (drawn twice for bold effect)
  if (mqttConnected)
  {
    display.setCursor(iconX, 0);     display.print('M');
    display.setCursor(iconX + 1, 0); display.print('M');
    iconX -= (ICON_WIDTH + 2);
  }

  // 'P' = POCSAG (drawn twice for bold effect)
  if (modePocsagEnabled)
  {
    display.setCursor(iconX, 0);     display.print('P');
    display.setCursor(iconX + 1, 0); display.print('P');
    iconX -= (ICON_WIDTH + 2);
  }


  // 'T' = Telegram (drawn twice for bold effect)
  if (telegramEnabled)
  {
    display.setCursor(iconX, 0);     display.print('T');
    display.setCursor(iconX + 1, 0); display.print('T');
    iconX -= (ICON_WIDTH + 2);
  }

  // 'B' = BrandMeister/DMR logged in (drawn twice for bold effect)
  if (dmrLoggedIn)
  {
    display.setCursor(iconX, 0);     display.print('B');
    display.setCursor(iconX + 1, 0); display.print('B');
    iconX -= (ICON_WIDTH + 2);
  }

  // 'D' = DAPNET/POCSAG pager logged in (drawn twice for bold effect)
  if (dapnetLoggedIn)
  {
    display.setCursor(iconX, 0);     display.print('D');
    display.setCursor(iconX + 1, 0); display.print('D');
    iconX -= (ICON_WIDTH + 2);
  }

  // ===== DIVIDER LINE 1 =====
  display.drawLine(0, 10, OLED_WIDTH - 1, 10, SSD1306_WHITE);

  // ===== MIDDLE SECTION (y=12 to y=48) =====
  // --- Smarter mode cycling: show all enabled modes, cycling if more than one ---
  int enabledModes[6];
  int enabledCount = 0;
  if (modeDmrEnabled)    enabledModes[enabledCount++] = 0;
  if (modeDstarEnabled)  enabledModes[enabledCount++] = 1;
  if (modeYsfEnabled)    enabledModes[enabledCount++] = 2;
  if (modeP25Enabled)    enabledModes[enabledCount++] = 3;
  if (modeNxdnEnabled)   enabledModes[enabledCount++] = 4;
  if (modePocsagEnabled) enabledModes[enabledCount++] = 5;
  if (dapnetEnabled)     enabledModes[enabledCount++] = 6;

  static unsigned long lastModeCycle = 0;
  static int modeCycleIdx = 0;
  const unsigned long MODE_CYCLE_INTERVAL = 2500; // ms
  unsigned long now2 = millis();
  if (enabledCount > 1) {
    if (now2 - lastModeCycle > MODE_CYCLE_INTERVAL) {
      modeCycleIdx = (modeCycleIdx + 1) % enabledCount;
      lastModeCycle = now2;
    }
  } else {
    modeCycleIdx = 0;
  }

  if (enabledCount == 0) {
    // No modes enabled - centered messages
    String line1 = "No mode activated";
    String line2 = "Enable mode in web";
    display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((OLED_WIDTH - w) / 2, 20);
    display.print(line1);
    display.getTextBounds(line2, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((OLED_WIDTH - w) / 2, 30);
    display.print(line2);
    display.setCursor(0, 40);
    display.print("Heap:");
    display.print(ESP.getFreeHeap() / 1024);
    display.print("KB Req:");
    display.print(requestCount);
  } else {
    int mode = enabledModes[modeCycleIdx];
    switch (mode) {
      case 0:
        display.setCursor(0, 14);
        display.println("DMR Mode Active");
        drawDMRmiddle();
        break;
      case 1:
        display.setCursor(0, 14);
        display.println("D-Star Mode Active");
        drawDSTARmiddle();
        break;
      case 2:
        display.setCursor(0, 14);
        display.println("YSF Mode Active");
        drawYSFmiddle();
        break;
      case 3:
        display.setCursor(0, 14);
        display.println("P25 Mode Active");
        drawP25middle();
        break;
      case 4:
        display.setCursor(0, 14);
        display.println("NXDN Mode Active");
        drawNXDNmiddle();
        break;
      case 5:
        display.setCursor(0, 14);
        display.println("POCSAG Mode Active");
        drawPOCSAGmiddle();
        break;
      case 6:
        display.setCursor(0, 14);
        display.println("DAPNET Mode Active");
        drawDAPNETmiddle();
        break;
      }
  }

  // ===== DIVIDER LINE 2 =====
  display.drawLine(0, 50, OLED_WIDTH - 1, 50, SSD1306_WHITE);

  // ===== BOTTOM BAR (y=54): Cycling network info =====
  // Advance cycle timer
  unsigned long now = millis();
  if (now - lastBottomCycle >= BOTTOM_CYCLE_INTERVAL)
  {
    oledBottomCycle++;
    lastBottomCycle = now;
  }

  // Build list of available bottom lines
  String bottomLines[5];
  bool bottomCenter[5];
  int bottomCount = 0;

  if (WiFi.status() == WL_CONNECTED)
  {
    bottomLines[bottomCount] = "WiFi: " + WiFi.localIP().toString();
    bottomCenter[bottomCount] = false;
    bottomCount++;
  }
  if (ethConnected)
  {
    bottomLines[bottomCount] = "ETH: " + ETH.localIP().toString();
    bottomCenter[bottomCount] = false;
    bottomCount++;
  }
  if (softAPActive)
  {
    bottomLines[bottomCount] = "AP: " + WiFi.softAPIP().toString();
    bottomCenter[bottomCount] = false;
    bottomCount++;
  }
  // add uptime
    // Uptime at y=26
  // unsigned long uptimeSec = millis() / 1000;
  // unsigned long hrs = uptimeSec / 3600;
  // unsigned long mins = (uptimeSec % 3600) / 60;
  // unsigned long secs = uptimeSec % 60;
  // display.setCursor(0, 26);
  // display.print("Up: ");
  // if (hrs > 0)
  // {
  //   display.print(hrs);
  //   display.print("h ");
  // }
  // display.print(mins);
  // display.print("m ");
  // display.print(secs);
  // display.println("s");

  // Always add callsign
  bottomLines[bottomCount] = userCallsign + " - ESP32 HS";
  bottomCenter[bottomCount] = true;
  bottomCount++;

  // If no network at all and no AP, show "No Network" instead of just callsign
  if (WiFi.status() != WL_CONNECTED && !ethConnected && !softAPActive)
  {
    // Replace the list: just "No Network" and callsign
    bottomLines[0] = "No Network";
    bottomCenter[0] = true;
    bottomLines[1] = userCallsign + " - ESP32 HS";
    bottomCenter[1] = true;
    bottomCount = 2;
  }

  int idx = oledBottomCycle % bottomCount;
  String bottomLine = bottomLines[idx];

  if (bottomCenter[idx])
  {
    display.getTextBounds(bottomLine, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((OLED_WIDTH - w) / 2, 54);
  }
  else
  {
    display.setCursor(0, 54);
  }
  display.print(bottomLine);

  display.display();
}

void displayWiFiStatus()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.println("WiFi Status:");
  display.println("");

  if (WiFi.status() == WL_CONNECTED)
  {
    display.println("Connected!");
    display.println("");
    display.print("SSID: ");
    display.println(WiFi.SSID());
    display.print("IP: ");
    display.println(WiFi.localIP());
  }
  else
  {
    display.println("Connecting...");
  }

  display.display();
}

void displayDownloadProgress()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Determine which file is being downloaded
  if (csvDownloadActive)
  {
    // File type at top (small text, centered)
    String fileType = "RadioID CSV";
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(fileType, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 0);
    display.print(fileType);

    // Progress percentage (medium size, centered)
    String percentText = String(csvDownloadProgress) + "%";
    display.setTextSize(2);
    display.getTextBounds(percentText, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 12);
    display.print(percentText);
    display.setTextSize(1);

    // Progress bar (20px height)
    int barWidth = 124;
    int barHeight = 20;
    int barY = 32;
    int filledWidth = (barWidth * csvDownloadProgress) / 100;
    display.drawRect(2, barY, barWidth, barHeight, SSD1306_WHITE);
    if (filledWidth > 0)
    {
      display.fillRect(2, barY, filledWidth, barHeight, SSD1306_WHITE);
    }

    // Bytes transferred at bottom (centered)
    float mbWritten = csvBytesWritten / 1048576.0;
    float mbTotal = csvBytesTotal / 1048576.0;
    String sizeText = String(mbWritten, 1) + " / " + String(mbTotal, 1) + " MB";
    display.getTextBounds(sizeText, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 56);
    display.print(sizeText);
  }
  else if (sqliteDownloadActive)
  {
    // File type at top (small text, centered)
    String fileType = "SQLite DB";
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(fileType, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 0);
    display.print(fileType);

    // Progress percentage (medium size, centered)
    String percentText = String(sqliteDownloadProgress) + "%";
    display.setTextSize(2);
    display.getTextBounds(percentText, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 12);
    display.print(percentText);
    display.setTextSize(1);

    // Progress bar (20px height)
    int barWidth = 124;
    int barHeight = 20;
    int barY = 32;
    int filledWidth = (barWidth * sqliteDownloadProgress) / 100;
    display.drawRect(2, barY, barWidth, barHeight, SSD1306_WHITE);
    if (filledWidth > 0)
    {
      display.fillRect(2, barY, filledWidth, barHeight, SSD1306_WHITE);
    }

    // Bytes transferred at bottom (centered)
    float mbWritten = sqliteBytesWritten / 1048576.0;
    float mbTotal = sqliteBytesTotal / 1048576.0;
    String sizeText = String(mbWritten, 1) + " / " + String(mbTotal, 1) + " MB";
    display.getTextBounds(sizeText, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 56);
    display.print(sizeText);
  }
  else if (espFirmwareUpdateActive)
  {
    // ESP32 firmware type at top
    String fwType = "ESP32 Firmware";
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(fwType, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 0);
    display.print(fwType);

    // Progress percentage (large)
    String percentText = String(espFirmwareUpdateProgress) + "%";
    display.setTextSize(2);
    display.getTextBounds(percentText, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 12);
    display.print(percentText);
    display.setTextSize(1);

    // Progress bar
    int barWidth = 124;
    int barHeight = 20;
    int barY = 32;
    int filledWidth = (barWidth * espFirmwareUpdateProgress) / 100;
    display.drawRect(2, barY, barWidth, barHeight, SSD1306_WHITE);
    if (filledWidth > 0)
    {
      display.fillRect(2, barY, filledWidth, barHeight, SSD1306_WHITE);
    }

    // Bytes transferred
    float kbWritten = espFirmwareBytesWritten / 1024.0;
    String sizeText;
    if (espFirmwareBytesTotal > 0) {
      float kbTotal = espFirmwareBytesTotal / 1024.0;
      sizeText = String(kbWritten, 1) + " / " + String(kbTotal, 1) + " KB";
    } else {
      sizeText = String(kbWritten, 1) + " KB";
    }
    display.getTextBounds(sizeText, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 56);
    display.print(sizeText);
  }

  display.display();
}

void displayFirmwareUpdateProgress()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Determine which firmware is being updated
  if (modemFirmwareUpdateActive)
  {
    // Firmware type at top
    String fwType = "Modem Firmware";
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(fwType, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 0);
    display.print(fwType);

    // Progress percentage (large)
    String percentText = String(modemFirmwareUpdateProgress) + "%";
    display.setTextSize(2);
    display.getTextBounds(percentText, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 12);
    display.print(percentText);
    display.setTextSize(1);

    // Progress bar
    int barWidth = 124;
    int barHeight = 20;
    int barY = 32;
    int filledWidth = (barWidth * modemFirmwareUpdateProgress) / 100;
    display.drawRect(2, barY, barWidth, barHeight, SSD1306_WHITE);
    if (filledWidth > 0)
    {
      display.fillRect(2, barY, filledWidth, barHeight, SSD1306_WHITE);
    }

    // Bytes transferred
    float kbWritten = modemFirmwareBytesWritten / 1024.0;
    String sizeText;
    if (modemFirmwareBytesTotal > 0) {
      float kbTotal = modemFirmwareBytesTotal / 1024.0;
      sizeText = String(kbWritten, 1) + " / " + String(kbTotal, 1) + " KB";
    } else {
      sizeText = String(kbWritten, 1) + " KB";
    }
    display.getTextBounds(sizeText, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 56);
    display.print(sizeText);
  }
  else if (espFirmwareUpdateActive)
  {
    // ESP32 firmware type at top
    String fwType = "ESP32 Firmware";
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(fwType, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 0);
    display.print(fwType);

    // Progress percentage (large)
    String percentText = String(espFirmwareUpdateProgress) + "%";
    display.setTextSize(2);
    display.getTextBounds(percentText, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 12);
    display.print(percentText);
    display.setTextSize(1);

    // Progress bar
    int barWidth = 124;
    int barHeight = 20;
    int barY = 32;
    int filledWidth = (barWidth * espFirmwareUpdateProgress) / 100;
    display.drawRect(2, barY, barWidth, barHeight, SSD1306_WHITE);
    if (filledWidth > 0)
    {
      display.fillRect(2, barY, filledWidth, barHeight, SSD1306_WHITE);
    }

    // Bytes transferred
    float kbWritten = espFirmwareBytesWritten / 1024.0;
    String sizeText;
    if (espFirmwareBytesTotal > 0) {
      float kbTotal = espFirmwareBytesTotal / 1024.0;
      sizeText = String(kbWritten, 1) + " / " + String(kbTotal, 1) + " KB";
    } else {
      sizeText = String(kbWritten, 1) + " KB";
    }
    display.getTextBounds(sizeText, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 56);
    display.print(sizeText);
  }

  display.display();
}

void displayRebootMessage()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  // "Rebooting..." centered
  String msg = "Rebooting";
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 28);
  display.print(msg);

  display.display();
}
void displayFirmwareReadyMessage()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Title
  String title = "Firmware Ready";
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 10);
  display.print(title);

  // Message line 1
  String msg1 = "Download complete!";
  display.getTextBounds(msg1, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 28);
  display.print(msg1);

  // Message line 2
  String msg2 = "Click FLASH button";
  display.getTextBounds(msg2, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 40);
  display.print(msg2);

  // Message line 3
  String msg3 = "on web page";
  display.getTextBounds(msg3, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 52);
  display.print(msg3);

  display.display();
}

void displayArduinoOtaProgress()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Title at top (centered)
  String title = "Arduino OTA";
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 0);
  display.print(title);

  // Progress percentage (large, centered)
  String percentText = String(arduinoOtaProgress) + "%";
  display.setTextSize(2);
  display.getTextBounds(percentText, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 12);
  display.print(percentText);
  display.setTextSize(1);

  // Progress bar
  int barWidth = 124;
  int barHeight = 20;
  int barY = 32;
  int filledWidth = (barWidth * arduinoOtaProgress) / 100;
  display.drawRect(2, barY, barWidth, barHeight, SSD1306_WHITE);
  if (filledWidth > 0)
  {
    display.fillRect(2, barY, filledWidth, barHeight, SSD1306_WHITE);
  }

  // Bytes transferred at bottom (centered)
  float kbWritten = arduinoOtaBytesWritten / 1024.0;
  float kbTotal = arduinoOtaBytesTotal / 1024.0;
  String sizeText = String(kbWritten, 1) + " / " + String(kbTotal, 1) + " KB";
  display.getTextBounds(sizeText, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 56);
  display.print(sizeText);

  display.display();
}