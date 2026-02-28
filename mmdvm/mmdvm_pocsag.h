/*
 * MMDVM POCSAG Protocol Module
 * Handles POCSAG paging protocol processing
 */

#ifndef MMDVM_POCSAG_H
#define MMDVM_POCSAG_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "../include/config.h"

// Task handle
extern TaskHandle_t pocsagTaskHandle;

// Message queue handle (max 10 pending messages)
#define POCSAG_QUEUE_SIZE 10
#define POCSAG_MSG_MAX_LEN 80

// Queued POCSAG message structure
struct PocsagQueueItem {
  uint32_t ric;
  char message[POCSAG_MSG_MAX_LEN + 1];
  uint8_t functional;
};

extern QueueHandle_t pocsagQueue;

// Shadow display array — mirrors the live queue for web UI inspection
extern PocsagQueueItem pocsagQueueDisplay[POCSAG_QUEUE_SIZE];
extern volatile uint8_t pocsagQueueDisplayCount;

// Create the message queue (call unconditionally at startup)
void initPocsagQueue();

// Function to create and start POCSAG task
void initPocsagTask();

// POCSAG task function
void pocsagTask(void *parameter);

// Functional codes (also used by web API)
#define FUNCTIONAL_NUMERIC       0U
#define FUNCTIONAL_ALERT1        1U
#define FUNCTIONAL_ALERT2        2U
#define FUNCTIONAL_ALPHANUMERIC  3U

// Queue a POCSAG message for transmission (non-blocking, returns true if queued)
bool queuePocsagMessage(uint32_t ric, const String& message, uint8_t functional = FUNCTIONAL_ALPHANUMERIC);

// Send a POCSAG message to the modem (blocking ~3s, used internally by task)
void sendPocsagMessageToModem(uint32_t ric, const String& message, uint8_t functional = FUNCTIONAL_ALPHANUMERIC);

// Set modem to POCSAG mode and frequency
void setModemPocsagMode();

// True while POCSAG is actively using the modem (blocks DMR TX)
extern volatile bool pocsagTxInProgress;

#endif // MMDVM_POCSAG_H
