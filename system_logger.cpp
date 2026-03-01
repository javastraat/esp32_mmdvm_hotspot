/*
 * System Logger Implementation
 */

#include "system/system_logger.h"

extern String mqttLoggerTaskTopic;

// Circular buffer for log messages (fixed char arrays — no heap allocation)
char logBuffer[LOG_BUFFER_SIZE][LOG_LINE_MAX_LENGTH];
int logBufferIndex = 0;
int logBufferCount = 0;

// Mutex for thread-safe access
SemaphoreHandle_t logMutex = NULL;

/*
 * Initialize logger
 */
void initLogger()
{
  logMutex = xSemaphoreCreateMutex();
  Serial.println("[Logger] Initialized - capturing last 50 messages");
}

/*
 * Add a log message to circular buffer (thread-safe)
 */
void addLogMessage(const String &message)
{
  if (logMutex == NULL)
  {
    // If mutex not ready, just print to serial
    Serial.println(message);
    return;
  }

  // Take mutex
  if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE)
  {
    // Add to circular buffer
    strlcpy(logBuffer[logBufferIndex], message.c_str(), LOG_LINE_MAX_LENGTH);
    logBufferIndex = (logBufferIndex + 1) % LOG_BUFFER_SIZE;

    if (logBufferCount < LOG_BUFFER_SIZE)
    {
      logBufferCount++;
    }

    // Print to Serial while holding mutex (prevents interleaved output)
    Serial.println(message);

    // Release mutex
    xSemaphoreGive(logMutex);
  }
}

/*
 * Get all log messages as JSON array
 */
String getLogMessagesJSON()
{
  if (logMutex == NULL)
    return "[]";

  String json = "[";

  if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE)
  {
    // Calculate starting index (oldest message)
    int startIdx = (logBufferCount < LOG_BUFFER_SIZE) ? 0 : logBufferIndex;

    for (int i = 0; i < logBufferCount; i++)
    {
      int idx = (startIdx + i) % LOG_BUFFER_SIZE;

      if (i > 0)
        json += ",";

      // JSON-escape the log line character by character
      json += "\"";
      for (int j = 0; logBuffer[idx][j] != '\0'; j++) {
        char c = logBuffer[idx][j];
        if      (c == '\\') json += "\\\\";
        else if (c == '"')  json += "\\\"";
        else if (c == '\n') json += "\\n";
        else if (c == '\r') { /* skip */ }
        else                json += c;
      }
      json += "\"";
    }

    xSemaphoreGive(logMutex);
  }

  json += "]";
  return json;
}

/*
 * Get all log messages as HTML (for direct display)
 */
String getLogMessagesHTML()
{
  if (logMutex == NULL)
    return "";

  String html = "";

  if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE)
  {
    // Calculate starting index (oldest message)
    int startIdx = (logBufferCount < LOG_BUFFER_SIZE) ? 0 : logBufferIndex;

    for (int i = 0; i < logBufferCount; i++)
    {
      int idx = (startIdx + i) % LOG_BUFFER_SIZE;

      // HTML-escape the log line character by character
      for (int j = 0; logBuffer[idx][j] != '\0'; j++) {
        char c = logBuffer[idx][j];
        if      (c == '&') html += "&amp;";
        else if (c == '<') html += "&lt;";
        else if (c == '>') html += "&gt;";
        else               html += c;
      }
      html += "<br>";
    }

    xSemaphoreGive(logMutex);
  }

  return html;
}
