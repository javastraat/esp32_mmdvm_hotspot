/*
 * Telegram Bot Integration
 * Async message sending via FreeRTOS queue + WiFiClientSecure
 */

#ifndef SYSTEM_TELEGRAM_H
#define SYSTEM_TELEGRAM_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Task and queue handles
extern QueueHandle_t telegramQueue;
extern TaskHandle_t telegramTaskHandle;

// Runtime settings (loaded from NVS in main .ino)
extern bool telegramEnabled;
extern String telegramBotToken;
extern String telegramChatId;
extern bool telegramRicForwardEnabled;
extern String telegramRicList;
extern uint16_t telegramRicCooldown;

// Initialize the background Telegram send task
void initTelegramTask();

// Queue a message for async delivery (non-blocking, safe from any RTOS task)
bool sendTelegramMessage(const String &message);

// Synchronous send - blocks caller until done; use only from web handlers (test button)
bool sendTelegramSync(const String &message, String &errorOut);

// Called from queuePocsagMessage to check RIC and forward if configured
void checkTelegramRicForward(uint32_t ric, uint8_t functional, const char *message);

#endif // SYSTEM_TELEGRAM_H
