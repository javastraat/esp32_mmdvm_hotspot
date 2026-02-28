/*
 * utils.h - Common Utilities for ESP32 RTOS MMDVM Web Interface
 */

#ifndef WEB_UTILS_H
#define WEB_UTILS_H

#include <Arduino.h>

// Footer HTML - uses FOOTER_LINK defines from config.h
String getFooter() {
  String footer = "<div class='footer-links'>";
  footer += "<a href='" + String(FOOTER_LINK1_URL) + "' target='_blank'>" + String(FOOTER_LINK1_TEXT) + "</a>";
  footer += " | ";
  footer += "<a href='" + String(FOOTER_LINK2_URL) + "' target='_blank'>" + String(FOOTER_LINK2_TEXT) + "</a>";
  footer += " | ";
  footer += "<a href='" + String(FOOTER_LINK3_URL) + "' target='_blank'>" + String(FOOTER_LINK3_TEXT) + "</a>";
  footer += " | ";
  footer += "<a href='" + String(FOOTER_LINK4_URL) + "' target='_blank'>" + String(FOOTER_LINK4_TEXT) + "</a>";
  footer += "</div>";
  footer += "<div class='copyright'>" + String(COPYRIGHT_TEXT) + "</div>";
  return footer;
}

#endif // WEB_UTILS_H
