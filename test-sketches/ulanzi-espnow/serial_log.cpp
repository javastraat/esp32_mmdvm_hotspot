// serial_log.cpp — 8 KB ring-buffer serial capture.
// Thread-safe via FreeRTOS spinlock (safe from both cores and WiFi task context).
#include "serial_log.h"
#include <freertos/portmacro.h>

#define SERIAL_LOG_SIZE 8192

static char          _buf[SERIAL_LOG_SIZE];
static uint32_t      _total = 0;   // monotonic bytes-written counter
static portMUX_TYPE  _mux   = portMUX_INITIALIZER_UNLOCKED;

void serialLog(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char tmp[256];
  int n = vsnprintf(tmp, sizeof(tmp), fmt, args);
  va_end(args);
  if (n <= 0) return;
  if (n >= (int)sizeof(tmp)) n = sizeof(tmp) - 1;

  Serial.print(tmp);   // hardware serial (Arduino IDE monitor)

  portENTER_CRITICAL(&_mux);
  for (int i = 0; i < n; i++)
    _buf[(_total + i) % SERIAL_LOG_SIZE] = tmp[i];
  _total += (uint32_t)n;
  portEXIT_CRITICAL(&_mux);
}

void serialLogClear() {
  portENTER_CRITICAL(&_mux);
  _total = 0;
  portEXIT_CRITICAL(&_mux);
}

// Returns new content since 'cursor' as a JSON-safe string, updates *newCursor.
// Max 1024 bytes per call to stay within web task stack budget.
String serialLogQuery(uint32_t cursor, uint32_t* newCursor) {
  portENTER_CRITICAL(&_mux);
  uint32_t total = _total;
  portEXIT_CRITICAL(&_mux);

  // Stale cursor (e.g. after a clear) — restart from oldest available
  if (cursor > total) cursor = 0;

  uint32_t oldest = (total > SERIAL_LOG_SIZE) ? (total - SERIAL_LOG_SIZE) : 0;
  if (cursor < oldest) cursor = oldest;   // fell behind ring-buffer wrap

  uint32_t avail = total - cursor;
  const uint32_t MAX_CHUNK = 1024;
  if (avail > MAX_CHUNK) avail = MAX_CHUNK;

  // Build JSON-escaped string (worst case doubles in size)
  String out;
  out.reserve(avail * 2);
  for (uint32_t i = 0; i < avail; i++) {
    char c = _buf[(cursor + i) % SERIAL_LOG_SIZE];
    switch (c) {
      case '"':  out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n";  break;
      case '\r': out += "\\r";  break;
      case '\t': out += "\\t";  break;
      default:
        if ((uint8_t)c >= 32) out += c;
        break;
    }
  }
  *newCursor = cursor + avail;
  return out;
}
