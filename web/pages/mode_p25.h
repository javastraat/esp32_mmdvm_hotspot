/*
 * P25 Mode Configuration Page
 * Configure P25 protocol settings
 */

#ifndef WEB_MODE_P25_H
#define WEB_MODE_P25_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

String getModeP25PageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>P25 Configuration</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("mode-p25");

  html += "<div class='container'>";
  html += "<h1>P25 Configuration</h1>";
  html += "<p>Configure Project 25 (APCO-25) mode settings</p>";

  html += "<div class='admin-grid'>";

  html += "<div class='card'>";
  html += "<h3>Mode Status</h3>";
  html += "<div class='metric'><span class='metric-label'>Status:</span><span class='metric-value'>Disabled</span></div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enable P25 Mode:</span>";
  html += "<label class='switch'><input type='checkbox' disabled><span class='slider'></span></label>";
  html += "</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>NAC Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>NAC:</span>";
  html += "<input type='text' placeholder='293' maxlength='3' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>3-digit hexadecimal NAC code (default: 293)</p>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Radio ID:</span>";
  html += "<input type='text' placeholder='123456' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Talkgroup Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Default Talkgroup:</span>";
  html += "<input type='text' placeholder='10100' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Gateway Address:</span>";
  html += "<input type='text' placeholder='p25.example.com' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Gateway Port:</span>";
  html += "<input type='text' value='41000' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "</div>";

  html += "</div>";

  html += "<div class='info' style='margin-top:20px'>";
  html += "<strong>Note:</strong> P25 mode configuration is a placeholder and will be implemented in a future update.";
  html += "</div>";

  html += "</div>";
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_MODE_P25_H
