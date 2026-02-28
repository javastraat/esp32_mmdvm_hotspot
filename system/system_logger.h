/*
 * System Logger
 * Circular buffer for capturing and displaying serial output on web interface
 */

#ifndef SYSTEM_LOGGER_H
#define SYSTEM_LOGGER_H

#include <Arduino.h>
#include "../include/config.h"

// Circular buffer for log messages (fixed char arrays — no heap allocation)
extern char logBuffer[LOG_BUFFER_SIZE][LOG_LINE_MAX_LENGTH];
extern int logBufferIndex;
extern int logBufferCount;

// Mutex for thread-safe access
extern SemaphoreHandle_t logMutex;

// Initialize logger
void initLogger();

// Add a log message (thread-safe)
void addLogMessage(const String &message);

// Get all log messages as JSON array
String getLogMessagesJSON();

// Get all log messages as HTML
String getLogMessagesHTML();

#endif // SYSTEM_LOGGER_H
