/*
 * D-Star Mode Configuration Page
 * Configure D-Star protocol settings
 */

#ifndef WEB_MODE_DSTAR_H
#define WEB_MODE_DSTAR_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

String getModeDstarPageHTML()
{
  String html;
  html.reserve(28000);
  html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>D-Star Configuration</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("mode-dstar");

  html += "<div class='container'>";
  html += "<h1>D-Star Configuration</h1>";
  html += "<p>Configure D-Star digital voice mode settings</p>";

  html += "<div class='admin-grid'>";

  html += "<div class='card'>";
  html += "<h3>Mode Status</h3>";
  html += "<div class='metric'><span class='metric-label'>Status:</span><span class='metric-value'>Disabled</span></div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enable D-Star Mode:</span>";
  html += "<label class='switch'><input type='checkbox' disabled><span class='slider'></span></label>";
  html += "</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Call Sign Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Your Callsign:</span>";
  html += "<input type='text' placeholder='Enter callsign' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>RPT1 Callsign:</span>";
  html += "<input type='text' placeholder='RPT1 callsign' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>RPT2 Callsign:</span>";
  html += "<input type='text' placeholder='RPT2 callsign' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Reflector Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Reflector Address:</span>";
  html += "<input type='text' placeholder='ref.example.com' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Reflector Port:</span>";
  html += "<input type='text' value='20001' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Startup Reflector:</span>";
  html += "<input type='text' placeholder='REF001C' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "</div>";

  html += "</div>";

  html += "<div class='info' style='margin-top:20px'>";
  html += "<strong>Note:</strong> D-Star mode configuration is a placeholder and will be implemented in a future update.";
  html += "</div>";

  html += "</div>";
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_MODE_DSTAR_H
