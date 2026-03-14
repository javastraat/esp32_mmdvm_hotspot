/*
 * NXDN Mode Configuration Page
 * Configure NXDN protocol settings
 */

#ifndef WEB_MODE_NXDN_H
#define WEB_MODE_NXDN_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

String getModeNxdnPageHTML()
{
  String html;
  html.reserve(28000);
  html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>NXDN Configuration</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("mode-nxdn");

  html += "<div class='container'>";
  html += "<h1>NXDN Configuration</h1>";
  html += "<p>Configure NXDN digital voice mode settings</p>";

  html += "<div class='admin-grid'>";

  html += "<div class='card'>";
  html += "<h3>Mode Status</h3>";
  html += "<div class='metric'><span class='metric-label'>Status:</span><span class='metric-value'>Disabled</span></div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enable NXDN Mode:</span>";
  html += "<label class='switch'><input type='checkbox' disabled><span class='slider'></span></label>";
  html += "</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>RAN Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>RAN:</span>";
  html += "<input type='text' value='1' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>RAN code (0-63, default: 1)</p>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Radio ID:</span>";
  html += "<input type='text' placeholder='123456' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Network Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Gateway Address:</span>";
  html += "<input type='text' placeholder='nxdn.example.com' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Gateway Port:</span>";
  html += "<input type='text' value='41400' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Default Talkgroup:</span>";
  html += "<input type='text' placeholder='10200' style='width: 120px; padding-right: 8px;' disabled>";
  html += "</div>";
  html += "</div>";

  html += "</div>";

  html += "<div class='info' style='margin-top:20px'>";
  html += "<strong>Note:</strong> NXDN mode configuration is a placeholder and will be implemented in a future update.";
  html += "</div>";

  html += "</div>";
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_MODE_NXDN_H
