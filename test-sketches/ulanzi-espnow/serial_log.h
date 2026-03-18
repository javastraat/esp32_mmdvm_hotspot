#pragma once
// serial_log.h — Ring-buffer serial capture.
// serialLog() writes to both hardware Serial AND a 8 KB ring buffer.
// The web /serial page polls /api/serial/log?cursor=N to get new output.
//
// Usage: call LOG(fmt, ...) instead of Serial.printf(fmt, ...).
//        Serial.begin() must still be called once in setup() before any LOG().

#include <Arduino.h>
#include <stdarg.h>

void   serialLog(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void   serialLogClear();
String serialLogQuery(uint32_t cursor, uint32_t* newCursor);

// LOG  — always captured (normal operational messages)
// DLOG — only captured when debugLogEnabled is true (verbose/repetitive messages)
#define LOG(fmt, ...)  serialLog(fmt, ##__VA_ARGS__)
#define DLOG(fmt, ...) do { if (debugLogEnabled) serialLog(fmt, ##__VA_ARGS__); } while (0)

extern bool debugLogEnabled;  // defined in ulanzi-espnow.ino, NVS key "debug_log"
