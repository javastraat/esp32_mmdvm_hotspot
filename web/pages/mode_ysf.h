/*
 * YSF/Fusion Mode Configuration Page
 * Configure YSF protocol settings
 */

#ifndef WEB_MODE_YSF_H
#define WEB_MODE_YSF_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

String getModeYsfPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>YSF Configuration</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("mode-ysf");

  html += "<div class='container'>";
  html += "<h1>YSF/Fusion Configuration</h1>";
  html += "<p>Configure Yaesu System Fusion (C4FM) mode settings</p>";

  html += "<div class='admin-grid'>";

  html += "<div class='card'>";
  html += "<h3>Mode Status</h3>";
  html += "<div class='metric'><span class='metric-label'>Status:</span><span class='metric-value'>Disabled</span></div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enable YSF Mode:</span>";
  html += "<label class='switch'><input type='checkbox' disabled><span class='slider'></span></label>";
  html += "</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Network Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Gateway Address:</span>";
  html += "<input type='text' placeholder='ysf.example.com' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Gateway Port:</span>";
  html += "<input type='text' value='42000' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Callsign:</span>";
  html += "<input type='text' placeholder='Enter callsign' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Startup Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Startup Reflector:</span>";
  html += "<input type='text' placeholder='YSF123' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>TX Frequency:</span>";
  html += "<input type='text' placeholder='430.0000' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>RX Frequency:</span>";
  html += "<input type='text' placeholder='430.0000' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "</div>";

  html += "</div>";

  html += "<div class='info' style='margin-top:20px'>";
  html += "<strong>Note:</strong> YSF mode configuration is a placeholder and will be implemented in a future update.";
  html += "</div>";

  html += "</div>";
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_MODE_YSF_H
