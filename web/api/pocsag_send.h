/*
 * API Endpoint: /api/pocsag/send
 * Accepts POST requests to send a POCSAG message
 * Expects: ric (uint), message (string)
 * Returns: JSON { success: true/false, error: "..." }
 */

#ifndef WEB_API_POCSAG_SEND_H
#define WEB_API_POCSAG_SEND_H

#include <Arduino.h>
#include <WebServer.h>
#include "include/config.h"


#include "mmdvm/mmdvm_pocsag.h"

void handleApiPocsagSend(WebServer &server) {
  // Rate limiting: max POCSAG_API_RATE_LIMIT requests per POCSAG_API_RATE_WINDOW_MS
  static unsigned long rateWindowStart = 0;
  static uint8_t rateCount = 0;
  unsigned long now = millis();
  if (now - rateWindowStart >= POCSAG_API_RATE_WINDOW_MS) {
    rateWindowStart = now;
    rateCount = 0;
  }
  if (rateCount >= POCSAG_API_RATE_LIMIT) {
    server.send(429, "application/json", "{\"success\":false,\"error\":\"Too many requests\"}");
    return;
  }
  rateCount++;

  //Serial.println("[DEBUG] Entered handleApiPocsagSend");
  if (server.method() != HTTP_POST) {
    Serial.println("[DEBUG] Not a POST request");
    server.send(405, "application/json", "{\"success\":false,\"error\":\"Method Not Allowed\"}");
    return;
  }
  if (!server.hasArg("ric") || !server.hasArg("message")) {
    Serial.println("[DEBUG] Missing ric or message argument");
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing ric or message\"}");
    return;
  }
  uint32_t ric = (uint32_t)server.arg("ric").toInt();
  String message = server.arg("message");

  // Optional: encoding=numeric or encoding=alpha (default alpha)
  uint8_t functional = FUNCTIONAL_ALPHANUMERIC;
  if (server.hasArg("encoding") && server.arg("encoding") == "numeric")
    functional = FUNCTIONAL_NUMERIC;

  Serial.printf("[API] POCSAG send: RIC=%u, ENC=%s, MSG=%s\n",
    ric, (functional == FUNCTIONAL_NUMERIC) ? "numeric" : "alpha", message.c_str());

  if (queuePocsagMessage(ric, message, functional)) {
    UBaseType_t pending = uxQueueMessagesWaiting(pocsagQueue);
    String resp = "{\"success\":true,\"queued\":" + String(pending) + "}";
    server.send(200, "application/json", resp);
  } else {
    server.send(503, "application/json", "{\"success\":false,\"error\":\"Queue full or not ready\"}");
  }
}

#endif // WEB_API_POCSAG_SEND_H
