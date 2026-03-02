/*
 * web_handlers_telegram_settings.cpp - Telegram Settings API Routes
 *
 * Routes:
 *   POST /api/save-telegram-service   enable / disable Telegram
 *   POST /api/reset-telegram-service  reset to default
 *   POST /api/save-telegram-bot       save bot token + chat ID
 *   POST /api/reset-telegram-bot      clear bot credentials
 *   POST /api/telegram-test           send a test message (sync, returns result)
 *   POST /api/save-telegram-ric       save RIC forward settings
 *   POST /api/reset-telegram-ric      reset RIC forward settings to default
 *
 * None of these require a reboot — Telegram calls are stateless HTTP.
 */

#include "system/web_handlers_telegram_settings.h"
#include "system/system_webserver.h"
#include "system/system_logger.h"
#include "system/system_telegram.h"
#include "include/config.h"

extern void saveSettings();

void registerTelegramSettingsRoutes()
{
  // ── Service enable / disable ──────────────────────────────────────────────

  server.on("/api/save-telegram-service", HTTP_POST, []() {
    if (!server.hasArg("enabled")) {
      server.send(400, "text/plain", "ERROR: Missing parameter");
      return;
    }
    telegramEnabled = (server.arg("enabled") == "1");
    saveSettings();
    // Start the task on-demand if it was never initialized (disabled at boot)
    if (telegramEnabled && telegramTaskHandle == NULL) {
      initTelegramTask();
    }
    addLogMessage("[Telegram] Service: " + String(telegramEnabled ? "Enabled" : "Disabled"));
    server.send(200, "text/plain", "Telegram " + String(telegramEnabled ? "enabled" : "disabled"));
  });

  server.on("/api/reset-telegram-service", HTTP_POST, []() {
    telegramEnabled = TELEGRAM_ENABLED;
    saveSettings();
    addLogMessage("[Telegram] Service reset to default");
    server.send(200, "text/plain", "Telegram service reset to default");
  });

  // ── Bot credentials ───────────────────────────────────────────────────────

  server.on("/api/save-telegram-bot", HTTP_POST, []() {
    if (!server.hasArg("token") || !server.hasArg("chatid")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }
    String token  = server.arg("token");
    String chatid = server.arg("chatid");
    if (token.length() < 10) {
      server.send(400, "text/plain", "ERROR: Bot token too short");
      return;
    }
    if (chatid.length() == 0) {
      server.send(400, "text/plain", "ERROR: Chat ID cannot be empty");
      return;
    }
    telegramBotToken = token;
    telegramChatId   = chatid;
    saveSettings();
    addLogMessage("[Telegram] Bot credentials saved");
    server.send(200, "text/plain", "Bot credentials saved");
  });

  server.on("/api/reset-telegram-bot", HTTP_POST, []() {
    telegramBotToken = TELEGRAM_BOT_TOKEN;
    telegramChatId   = TELEGRAM_CHAT_ID;
    saveSettings();
    addLogMessage("[Telegram] Bot credentials cleared");
    server.send(200, "text/plain", "Bot credentials cleared");
  });

  // ── Test send (synchronous) ───────────────────────────────────────────────

  server.on("/api/telegram-test", HTTP_POST, []() {
    if (!telegramEnabled) {
      server.send(400, "text/plain", "ERROR: Telegram is disabled");
      return;
    }
    String err;
    bool ok = sendTelegramSync("Test message from ESP32 MMDVM Hotspot", err);
    if (ok) {
      addLogMessage("[Telegram] Test message sent successfully");
      server.send(200, "text/plain", "OK: Test message delivered");
    } else {
      addLogMessage("[Telegram] Test message failed: " + err);
      server.send(200, "text/plain", "ERROR: " + err);
    }
  });

  // ── RIC forward settings ──────────────────────────────────────────────────

  server.on("/api/save-telegram-ric", HTTP_POST, []() {
    if (!server.hasArg("ric_en") || !server.hasArg("ric_list") || !server.hasArg("cooldown")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }
    telegramRicForwardEnabled = (server.arg("ric_en") == "1");
    telegramRicList           = server.arg("ric_list");
    int cd = server.arg("cooldown").toInt();
    if (cd < 10 || cd > 3600) {
      server.send(400, "text/plain", "ERROR: Cooldown must be 10-3600 seconds");
      return;
    }
    telegramRicCooldown = (uint16_t)cd;
    saveSettings();
    addLogMessage("[Telegram] RIC forward settings saved");
    server.send(200, "text/plain", "RIC forward settings saved");
  });

  server.on("/api/reset-telegram-ric", HTTP_POST, []() {
    telegramRicForwardEnabled = TELEGRAM_RIC_FORWARD_ENABLED;
    telegramRicList           = TELEGRAM_RIC_LIST;
    telegramRicCooldown       = TELEGRAM_RIC_COOLDOWN;
    saveSettings();
    addLogMessage("[Telegram] RIC forward settings reset to default");
    server.send(200, "text/plain", "RIC forward settings reset to default");
  });
}
