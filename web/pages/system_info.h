/*
 * system_info.h - System Information Page for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_PAGES_SYSTEM_INFO_H
#define WEB_PAGES_SYSTEM_INFO_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP.h>
#include "../common/css.h"
#include "../common/navigation.h"
#include "../common/utils.h"
#include "../common/server_utils.h"

// External variables
extern WebServer server;
extern String modemFirmwareVersion;

void handleSystemInfo() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>System Info - ESP32 MMDVM</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".admin-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 20px 0; }";
  html += ".metric { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #eee; }";
  html += ".metric:last-child { border-bottom: none; }";
  html += ".metric-label { font-weight: bold; color: #555; }";
  html += ".metric-value { color: #333; }";
  html += ".uptime { color: #007bff; font-weight: bold; }";
  html += "</style></head><body>";
  html += getNavigation("systeminfo");
  html += "<div class='container'>";
  html += "<h1>System Information</h1>";

  html += "<div class='admin-grid'>";

  // System Information Card
  html += "<div class='card'>";
  html += "<h3>System Information</h3>";

  // Uptime calculation
  unsigned long uptimeSeconds = millis() / 1000;
  unsigned long days = uptimeSeconds / 86400;
  unsigned long hours = (uptimeSeconds % 86400) / 3600;
  unsigned long minutes = (uptimeSeconds % 3600) / 60;
  unsigned long seconds = uptimeSeconds % 60;
  String uptimeStr = "";
  if (days > 0) uptimeStr += String(days) + "d ";
  if (hours > 0 || days > 0) uptimeStr += String(hours) + "h ";
  uptimeStr += String(minutes) + "m " + String(seconds) + "s";

  html += "<div class='metric'><span class='metric-label'>Uptime:</span><span class='metric-value uptime'>" + uptimeStr + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Chip Model:</span><span class='metric-value'>" + String(ESP.getChipModel()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Chip Revision:</span><span class='metric-value'>" + String(ESP.getChipRevision()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>CPU Cores:</span><span class='metric-value'>" + String(ESP.getChipCores()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>CPU Frequency:</span><span class='metric-value'>" + String(ESP.getCpuFreqMHz()) + " MHz</span></div>";
  html += "<div class='metric'><span class='metric-label'>Free Heap:</span><span class='metric-value'>" + String(ESP.getFreeHeap()/1024.0, 1) + " KB (" + String(ESP.getFreeHeap()*100/ESP.getHeapSize()) + "%)</span></div>";
  html += "<div class='metric'><span class='metric-label'>Min Free Heap:</span><span class='metric-value'>" + String(ESP.getMinFreeHeap()/1024.0, 1) + " KB</span></div>";
  html += "<div class='metric'><span class='metric-label'>Heap Size:</span><span class='metric-value'>" + String(ESP.getHeapSize()/1024.0, 1) + " KB</span></div>";

  // PSRAM info (if available)
  if (ESP.getPsramSize() > 0) {
    html += "<div class='metric'><span class='metric-label'>PSRAM Size:</span><span class='metric-value'>" + String(ESP.getPsramSize()/1024/1024) + " MB</span></div>";
    html += "<div class='metric'><span class='metric-label'>Free PSRAM:</span><span class='metric-value'>" + String(ESP.getFreePsram()/1024.0, 1) + " KB</span></div>";
  }

  html += "<div class='metric'><span class='metric-label'>Flash Size:</span><span class='metric-value'>" + String(ESP.getFlashChipSize()/1024/1024) + " MB</span></div>";
  html += "<div class='metric'><span class='metric-label'>Flash Speed:</span><span class='metric-value'>" + String(ESP.getFlashChipSpeed()/1000000) + " MHz</span></div>";
  html += "<div class='metric'><span class='metric-label'>Sketch Size:</span><span class='metric-value'>" + String(ESP.getSketchSize()/1024.0, 1) + " KB</span></div>";
  html += "<div class='metric'><span class='metric-label'>Free Sketch Space:</span><span class='metric-value'>" + String(ESP.getFreeSketchSpace()/1024.0, 1) + " KB</span></div>";
  html += "<div class='metric'><span class='metric-label'>SDK Version:</span><span class='metric-value'>" + String(ESP.getSdkVersion()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Firmware Version:</span><span class='metric-value'>" + String(FIRMWARE_VERSION) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Build Date:</span><span class='metric-value'>" + String(__DATE__) + " " + String(__TIME__) + "</span></div>";
  html += "</div>";

  // Modem Information Card - Parse the firmware version string
  html += "<div class='card'>";
  html += "<h3>Modem Information</h3>";

  // Parse modem firmware version string
  // Example: "MMDVM_HS_Hat-v1.5.2 20201108 14.7456MHz ADF7021 FW by CA6JAU GitID #89daa20"
  String hardware = "Unknown";
  String version = "Unknown";
  String buildDate = "Unknown";
  String crystal = "Unknown";
  String transceiver = "Unknown";
  String author = "Unknown";
  String gitId = "Unknown";

  if (modemFirmwareVersion != "Unknown" && modemFirmwareVersion.length() > 0) {
    String fwStr = modemFirmwareVersion;

    // Extract hardware (before "-v" or first space)
    int vPos = fwStr.indexOf("-v");
    if (vPos > 0) {
      hardware = fwStr.substring(0, vPos);
      fwStr = fwStr.substring(vPos + 2); // Skip "-v"
    } else {
      int spacePos = fwStr.indexOf(' ');
      if (spacePos > 0) {
        hardware = fwStr.substring(0, spacePos);
        fwStr = fwStr.substring(spacePos + 1);
      }
    }

    // Extract version (digits and dots until space)
    int spacePos = fwStr.indexOf(' ');
    if (spacePos > 0) {
      version = fwStr.substring(0, spacePos);
      fwStr = fwStr.substring(spacePos + 1);
    }

    // Extract build date (8 digits, may have suffix like _WPSD)
    spacePos = fwStr.indexOf(' ');
    if (spacePos > 0) {
      String dateStr = fwStr.substring(0, spacePos);

      // Check if first 8 characters are digits (YYYYMMDD)
      if (dateStr.length() >= 8) {
        bool isValidDate = true;
        for (int i = 0; i < 8; i++) {
          if (!isdigit(dateStr.charAt(i))) {
            isValidDate = false;
            break;
          }
        }

        if (isValidDate) {
          // Format YYYYMMDD to DD-MM-YYYY (ignore any suffix like _WPSD)
          buildDate = dateStr.substring(6, 8) + "-" + dateStr.substring(4, 6) + "-" + dateStr.substring(0, 4);
        }
      }
      fwStr = fwStr.substring(spacePos + 1);
    }

    // Extract crystal frequency (number followed by MHz)
    int mhzPos = fwStr.indexOf("MHz");
    if (mhzPos > 0) {
      int startPos = 0;
      for (int i = mhzPos - 1; i >= 0; i--) {
        if (fwStr.charAt(i) == ' ') {
          startPos = i + 1;
          break;
        }
      }
      crystal = fwStr.substring(startPos, mhzPos + 3);
      fwStr = fwStr.substring(mhzPos + 3);
    }

    // Extract transceiver (word after MHz, before " FW")
    fwStr.trim();
    int fwPos = fwStr.indexOf(" FW");
    if (fwPos > 0) {
      // Get the first word (transceiver name)
      spacePos = fwStr.indexOf(' ');
      if (spacePos > 0) {
        transceiver = fwStr.substring(0, spacePos);
      } else {
        // No space found, use everything before " FW"
        transceiver = fwStr.substring(0, fwPos);
      }
      fwStr = fwStr.substring(fwPos + 3); // Skip " FW"
    }

    // Extract author (after "by " before " GitID")
    int byPos = fwStr.indexOf("by ");
    int gitPos = fwStr.indexOf(" GitID");
    if (byPos >= 0 && gitPos > byPos) {
      author = fwStr.substring(byPos + 3, gitPos);
      author.trim();
    }

    // Extract Git ID (after "GitID ")
    gitPos = fwStr.indexOf("GitID ");
    if (gitPos >= 0) {
      gitId = fwStr.substring(gitPos + 6);
      gitId.trim();
    }
  }

  html += "<div class='metric'><span class='metric-label'>Hardware:</span><span class='metric-value'>" + hardware + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Firmware Version:</span><span class='metric-value'>" + version + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Build Date:</span><span class='metric-value'>" + buildDate + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Crystal:</span><span class='metric-value'>" + crystal + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Transceiver:</span><span class='metric-value'>" + transceiver + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Author:</span><span class='metric-value'>" + author + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Git ID:</span><span class='metric-value'>" + gitId + "</span></div>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  html += getFooter();
  html += "</div>"; // Close container
  html += "</body></html>";

  server.send(200, "text/html; charset=UTF-8", html);
}

#endif // WEB_PAGES_SYSTEM_INFO_H
