/*
 * admin_system.h - Admin System Page
 * Displays system information, hardware metrics, and modem info
 */

#ifndef WEB_PAGES_ADMIN_SYSTEM_H
#define WEB_PAGES_ADMIN_SYSTEM_H

#include <Arduino.h>
#include "admin_common.h"

// Forward declarations of external variables
extern String dmr_callsign;
extern String modemFirmwareVersion;
extern String firmwareVersion;
extern bool mmdvmReady;

void handleAdminSystem() {
  if (!checkAuthentication()) return;

  String html;
  html.reserve(15000);
  
  html = getAdminHeader("System Info", "admin");

  // Start admin grid container
  html += "<div class='admin-grid'>";

  // System Information Card
  html += "<div class='card'>";
  html += "<h3>System Information</h3>";
  
  // Calculate uptime
  unsigned long uptimeMillis = millis();
  unsigned long uptimeSecs = uptimeMillis / 1000;
  unsigned long uptimeMins = uptimeSecs / 60;
  unsigned long uptimeHours = uptimeMins / 60;
  unsigned long uptimeDays = uptimeHours / 24;
  unsigned long displayHours = uptimeHours % 24;
  unsigned long displayMins = uptimeMins % 60;
  unsigned long displaySecs = uptimeSecs % 60;

  html += "<div class='metric'>";
  html += "<span class='metric-label'>Uptime:</span>";
  html += "<span class='metric-value'>" + String(uptimeDays) + "d " + String(displayHours) + "h " + String(displayMins) + "m " + String(displaySecs) + "s</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Chip Model:</span>";
  html += "<span class='metric-value'>" + String(ESP.getChipModel()) + "</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Chip Revision:</span>";
  html += "<span class='metric-value'>" + String(ESP.getChipRevision()) + "</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>CPU Cores:</span>";
  html += "<span class='metric-value'>" + String(ESP.getChipCores()) + "</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>CPU Frequency:</span>";
  html += "<span class='metric-value'>" + String(ESP.getCpuFreqMHz()) + " MHz</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Free Heap:</span>";
  html += "<span class='metric-value'>" + String(ESP.getFreeHeap() / 1024.0, 1) + " KB (" + String(ESP.getFreeHeap() * 100 / ESP.getHeapSize()) + "%)</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Min Free Heap:</span>";
  html += "<span class='metric-value'>" + String(ESP.getMinFreeHeap() / 1024.0, 1) + " KB</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Heap Size:</span>";
  html += "<span class='metric-value'>" + String(ESP.getHeapSize() / 1024.0, 1) + " KB</span>";
  html += "</div>";
  
  // PSRAM info (if available)
  if (ESP.getPsramSize() > 0) {
    html += "<div class='metric'>";
    html += "<span class='metric-label'>PSRAM Size:</span>";
    html += "<span class='metric-value'>" + String(ESP.getPsramSize() / 1024 / 1024) + " MB</span>";
    html += "</div>";
    html += "<div class='metric'>";
    html += "<span class='metric-label'>Free PSRAM:</span>";
    html += "<span class='metric-value'>" + String(ESP.getFreePsram() / 1024.0, 1) + " KB</span>";
    html += "</div>";
  }
  
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Flash Size:</span>";
  html += "<span class='metric-value'>" + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Flash Speed:</span>";
  html += "<span class='metric-value'>" + String(ESP.getFlashChipSpeed() / 1000000) + " MHz</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Sketch Size:</span>";
  html += "<span class='metric-value'>" + String(ESP.getSketchSize() / 1024.0, 1) + " KB</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Free Sketch Space:</span>";
  html += "<span class='metric-value'>" + String(ESP.getFreeSketchSpace() / 1024.0, 1) + " KB</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>SDK Version:</span>";
  html += "<span class='metric-value'>" + String(ESP.getSdkVersion()) + "</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Firmware Version:</span>";
  html += "<span class='metric-value'>" + String(FIRMWARE_VERSION) + "</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Build Date:</span>";
  html += "<span class='metric-value'>" + String(__DATE__) + " " + String(__TIME__) + "</span>";
  html += "</div>";
  html += "</div>";


  // MMDVM Modem Info Card - Parse the firmware version string
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

  html += "<div class='metric'>";
  html += "<span class='metric-label'>Hardware:</span>";
  html += "<span class='metric-value'>" + hardware + "</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Firmware Version:</span>";
  html += "<span class='metric-value'>" + version + "</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Build Date:</span>";
  html += "<span class='metric-value'>" + buildDate + "</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Crystal:</span>";
  html += "<span class='metric-value'>" + crystal + "</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Transceiver:</span>";
  html += "<span class='metric-value'>" + transceiver + "</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Author:</span>";
  html += "<span class='metric-value'>" + author + "</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Git ID:</span>";
  html += "<span class='metric-value'>" + gitId + "</span>";
  html += "</div>";
  html += "</div>";

  // System Control Card
  html += "<div class='card'>";
  html += "<h3>System Control</h3>";
  html += "<p>Control basic system functions:</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='javascript:void(0)' onclick='rebootSystem()' class='btn btn-warning'>Reboot System</a>";
  html += "<a href='javascript:void(0)' onclick='restartServices()' class='btn btn-primary'>Restart Services</a>";
  html += "</div>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  // Warning message
  html += "<div class='info' style='background: var(--info-bg); border-left-color: #ffc107; color: var(--text-color);'>";
  html += "<strong>Warning:</strong> Some actions like reset and reboot will cause the system to restart. ";
  html += "Make sure you have saved any important configuration changes before proceeding.";
  html += "</div>";

  // Include shared JavaScript functions
  html += "<script>";
  html += "function rebootSystem() {";
  html += "  if (confirm('Are you sure you want to reboot the system? This will temporarily interrupt service.')) {";
  html += "    fetch('/reboot', {method: 'POST'}).then(() => {";
  html += "      alert('System is rebooting... Please wait 30 seconds before reconnecting.');";
  html += "      setTimeout(() => { window.location.href = '/'; }, 30000);";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function restartServices() {";
  html += "  if (confirm('Restart DMR and network services?')) {";
  html += "    fetch('/restart-services', {method: 'POST'}).then(() => {";
  html += "      alert('Services restarted successfully!');";
  html += "      setTimeout(() => { window.location.reload(); }, 2000);";
  html += "    });";
  html += "  }";
  html += "}";
  html += "</script>";

  html += getAdminFooter();
  server.send(200, "text/html; charset=UTF-8", html);
}

#endif // WEB_PAGES_ADMIN_SYSTEM_H
