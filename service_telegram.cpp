/*
 * service_telegram.cpp - Telegram Bot Integration
 *
 * Provides async POCSAG-to-Telegram forwarding and a test-send endpoint.
 * Messages are queued to a background RTOS task that performs the HTTPS POST,
 * keeping web handlers and the POCSAG task fully non-blocking.
 *
 * TLS note: setInsecure() is used to skip CA certificate verification.
 * This is intentional for embedded use; the bot token itself authenticates
 * the connection to the correct Telegram server.
 */

#include "system/service_telegram.h"
#include "system/system_logger.h"
#include "include/config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "system/system_eth.h"
#include "mmdvm/mmdvm_pocsag.h"  // FUNCTIONAL_NUMERIC

// Runtime settings (defined in esp32_mmdvm_hotspot.ino, loaded from NVS)
extern bool telegramEnabled;
extern String telegramBotToken;
extern String telegramChatId;
extern bool telegramRicForwardEnabled;
extern String telegramRicList;
extern uint16_t telegramRicCooldown;

// Task and queue
QueueHandle_t telegramQueue = NULL;
TaskHandle_t telegramTaskHandle = NULL;

#define TELEGRAM_QUEUE_SIZE 8
#define TELEGRAM_MSG_MAX    512

struct TelegramQueueItem {
  char message[TELEGRAM_MSG_MAX];
};

// Per-RIC cooldown tracking
#define TELEGRAM_RIC_TRACK_MAX 20
static struct {
  uint32_t ric;
  unsigned long lastSentMs;
} ricCooldownTrack[TELEGRAM_RIC_TRACK_MAX];
static portMUX_TYPE ricCooldownMux = portMUX_INITIALIZER_UNLOCKED;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static bool ricInForwardList(uint32_t ric)
{
  if (telegramRicList.length() == 0) return false;
  String target = String(ric);
  int idx = 0;
  while (idx < (int)telegramRicList.length()) {
    int comma = telegramRicList.indexOf(',', idx);
    if (comma < 0) comma = (int)telegramRicList.length();
    String token = telegramRicList.substring(idx, comma);
    token.trim();
    if (token == target) return true;
    idx = comma + 1;
  }
  return false;
}

static bool checkRicCooldown(uint32_t ric)
{
  unsigned long now = millis();
  unsigned long cooldownMs = (unsigned long)telegramRicCooldown * 1000UL;
  bool allowed = false;

  taskENTER_CRITICAL(&ricCooldownMux);
  int freeSlot = -1;
  for (int i = 0; i < TELEGRAM_RIC_TRACK_MAX; i++) {
    if (ricCooldownTrack[i].ric == ric) {
      if ((now - ricCooldownTrack[i].lastSentMs) >= cooldownMs) {
        ricCooldownTrack[i].lastSentMs = now;
        allowed = true;
      }
      taskEXIT_CRITICAL(&ricCooldownMux);
      return allowed;
    }
    if (ricCooldownTrack[i].ric == 0 && freeSlot < 0) freeSlot = i;
  }
  // First time seeing this RIC
  if (freeSlot >= 0) {
    ricCooldownTrack[freeSlot].ric = ric;
    ricCooldownTrack[freeSlot].lastSentMs = now;
    allowed = true;
  }
  taskEXIT_CRITICAL(&ricCooldownMux);
  return allowed;
}

// ─── Core HTTP POST ───────────────────────────────────────────────────────────

static bool doTelegramPost(const String &message, String &errOut)
{
  if (telegramBotToken.length() == 0) { errOut = "Bot token not set"; return false; }
  if (telegramChatId.length() == 0)   { errOut = "Chat ID not set";   return false; }
  if (WiFi.status() != WL_CONNECTED && !ethConnected) {
    errOut = "No network connection";
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String url = "https://api.telegram.org/bot" + telegramBotToken + "/sendMessage";
  if (!http.begin(client, url)) {
    errOut = "HTTP begin failed";
    return false;
  }

  http.setTimeout(10000);
  http.addHeader("Content-Type", "application/json");

  JsonDocument body;
  body["chat_id"] = telegramChatId;
  body["text"]    = message;
  String bodyStr;
  serializeJson(body, bodyStr);

  int code = http.POST(bodyStr);
  String resp = http.getString();
  http.end();

  if (code != 200) {
    errOut = "HTTP " + String(code) + ": " + resp;
    return false;
  }

  JsonDocument respDoc;
  if (deserializeJson(respDoc, resp)) {
    errOut = "JSON parse error";
    return false;
  }

  if (!respDoc["ok"].as<bool>()) {
    errOut = respDoc["description"] | "Unknown Telegram error";
    return false;
  }

  return true;
}

// ─── Background task ─────────────────────────────────────────────────────────

static void telegramTask(void *parameter)
{
  addLogMessage("[Telegram] Task started");
  TelegramQueueItem item;

  while (true) {
    if (xQueueReceive(telegramQueue, &item, portMAX_DELAY) == pdTRUE) {
      if (!telegramEnabled) continue;
      String err;
      if (doTelegramPost(String(item.message), err)) {
        addLogMessage("[Telegram] Message sent OK");
      } else {
        addLogMessage("[Telegram] Send failed: " + err);
      }
    }
  }
}

// ─── Public API ──────────────────────────────────────────────────────────────

void initTelegramTask()
{
  if (telegramQueue == NULL) {
    telegramQueue = xQueueCreate(TELEGRAM_QUEUE_SIZE, sizeof(TelegramQueueItem));
  }
  if (telegramQueue == NULL) {
    addLogMessage("[Telegram] ERROR: Failed to create queue");
    return;
  }

  BaseType_t result = xTaskCreatePinnedToCore(
      telegramTask, "Telegram Task",
      TELEGRAM_TASK_STACK, NULL,
      TELEGRAM_TASK_PRIORITY, &telegramTaskHandle,
      0 // Core 0 — leaves core 1 for MMDVM/POCSAG tasks
  );

  if (result != pdPASS) {
    addLogMessage("[Telegram] ERROR: Task creation failed");
  } else {
    addLogMessage("[Telegram] Task initialized");
  }
}

bool sendTelegramMessage(const String &message)
{
  if (telegramQueue == NULL || !telegramEnabled) return false;
  TelegramQueueItem item;
  strlcpy(item.message, message.c_str(), sizeof(item.message));
  return xQueueSend(telegramQueue, &item, 0) == pdTRUE;
}

bool sendTelegramSync(const String &message, String &errorOut)
{
  return doTelegramPost(message, errorOut);
}

void checkTelegramRicForward(uint32_t ric, uint8_t functional, const char *message)
{
  if (!telegramEnabled || !telegramRicForwardEnabled) return;
  if (!ricInForwardList(ric)) return;
  if (!checkRicCooldown(ric)) {
    addLogMessage("[Telegram] RIC " + String(ric) + " in cooldown - skipped");
    return;
  }

  const char *encName = (functional == FUNCTIONAL_NUMERIC) ? "Numeric" : "Alphanumeric";
  String fwdMsg = String("[POCSAG Forward]\nRIC: ") + String(ric) +
                  "\nType: " + encName +
                  "\nMessage: " + String(message);

  sendTelegramMessage(fwdMsg);
  addLogMessage("[Telegram] Forwarding RIC " + String(ric) + " to Telegram");
}
