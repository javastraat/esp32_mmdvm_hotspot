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
  html += "<span class='metric-label'>Firmware Version:</span>";
  html += "<span class='metric-value'>" + firmwareVersion + "</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Device Name:</span>";
  html += "<span class='metric-value'>" + dmr_callsign + "</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Uptime:</span>";
  html += "<span class='metric-value'>" + String(uptimeDays) + "d " + String(displayHours) + "h " + String(displayMins) + "m " + String(displaySecs) + "s</span>";
  html += "</div>";
  html += "</div>";

  // ESP32 Hardware Info Card
  html += "<div class='card'>";
  html += "<h3>ESP32 Hardware</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Chip Model:</span>";
  html += "<span class='metric-value'>" + String(ESP.getChipModel()) + "</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Cores:</span>";
  html += "<span class='metric-value'>" + String(ESP.getChipCores()) + "</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>CPU Frequency:</span>";
  html += "<span class='metric-value'>" + String(ESP.getCpuFreqMHz()) + " MHz</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Flash Size:</span>";
  html += "<span class='metric-value'>" + String(ESP.getFlashChipSize() / 1048576.0, 2) + " MB</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Free Heap:</span>";
  html += "<span class='metric-value'>" + String(ESP.getFreeHeap() / 1024.0, 2) + " KB</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Heap Size:</span>";
  html += "<span class='metric-value'>" + String(ESP.getHeapSize() / 1024.0, 2) + " KB</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Min Free Heap:</span>";
  html += "<span class='metric-value'>" + String(ESP.getMinFreeHeap() / 1024.0, 2) + " KB</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Max Alloc Heap:</span>";
  html += "<span class='metric-value'>" + String(ESP.getMaxAllocHeap() / 1024.0, 2) + " KB</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>PSRAM Size:</span>";
  html += "<span class='metric-value'>" + String(ESP.getPsramSize() / 1024.0, 2) + " KB</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Free PSRAM:</span>";
  html += "<span class='metric-value'>" + String(ESP.getFreePsram() / 1024.0, 2) + " KB</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Min Free PSRAM:</span>";
  html += "<span class='metric-value'>" + String(ESP.getMinFreePsram() / 1024.0, 2) + " KB</span>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Max Alloc PSRAM:</span>";
  html += "<span class='metric-value'>" + String(ESP.getMaxAllocPsram() / 1024.0, 2) + " KB</span>";
  html += "</div>";
  html += "</div>";

  // MMDVM Modem Info Card
  html += "<div class='card'>";
  html += "<h3>MMDVM Modem Information</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Status:</span>";
  html += "<span class='metric-value'><strong style='color: " + String(mmdvmReady ? "var(--success-color)" : "var(--danger-color)") + ";'>" + String(mmdvmReady ? "Ready" : "Not Connected") + "</strong></span>";
  html += "</div>";
  
  // Parse modem firmware version string for detailed info
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Firmware Version:</span>";
  html += "<span class='metric-value'>" + modemFirmwareVersion + "</span>";
  html += "</div>";
  
  // Parse modem firmware for details (format: "20201012 MMDVM_HS_HAT v1.5.2 12.2880MHz ADF7021 GitID #6e8f826")
  if (modemFirmwareVersion.length() > 0) {
    // Extract hardware type
    int hwStart = modemFirmwareVersion.indexOf("MMDVM");
    int hwEnd = modemFirmwareVersion.indexOf(" v", hwStart);
    if (hwStart != -1 && hwEnd != -1) {
      String hardware = modemFirmwareVersion.substring(hwStart, hwEnd);
      html += "<div class='metric'>";
      html += "<span class='metric-label'>Hardware:</span>";
      html += "<span class='metric-value'>" + hardware + "</span>";
      html += "</div>";
    }
    
    // Extract version number
    int verStart = modemFirmwareVersion.indexOf(" v");
    int verEnd = modemFirmwareVersion.indexOf(" ", verStart + 2);
    if (verStart != -1 && verEnd != -1) {
      String version = modemFirmwareVersion.substring(verStart + 2, verEnd);
      html += "<div class='metric'>";
      html += "<span class='metric-label'>Version:</span>";
      html += "<span class='metric-value'>" + version + "</span>";
      html += "</div>";
    }
    
    // Extract build date (YYYYMMDD at start)
    if (modemFirmwareVersion.length() >= 8) {
      String buildDate = modemFirmwareVersion.substring(0, 8);
      html += "<div class='metric'>";
      html += "<span class='metric-label'>Build Date:</span>";
      html += "<span class='metric-value'>" + buildDate + "</span>";
      html += "</div>";
    }
    
    // Extract crystal frequency
    int crystalStart = modemFirmwareVersion.indexOf("MHz");
    if (crystalStart != -1) {
      int crystalBegin = crystalStart;
      while (crystalBegin > 0 && (isdigit(modemFirmwareVersion[crystalBegin - 1]) || modemFirmwareVersion[crystalBegin - 1] == '.')) {
        crystalBegin--;
      }
      String crystal = modemFirmwareVersion.substring(crystalBegin, crystalStart + 3);
      html += "<div class='metric'>";
      html += "<span class='metric-label'>Crystal:</span>";
      html += "<span class='metric-value'>" + crystal + "</span>";
      html += "</div>";
    }
    
    // Extract transceiver chip
    int transceiverPos = modemFirmwareVersion.indexOf("ADF");
    if (transceiverPos == -1) transceiverPos = modemFirmwareVersion.indexOf("SI");
    if (transceiverPos != -1) {
      int transceiverEnd = modemFirmwareVersion.indexOf(" ", transceiverPos);
      String transceiver = modemFirmwareVersion.substring(transceiverPos, transceiverEnd != -1 ? transceiverEnd : modemFirmwareVersion.length());
      html += "<div class='metric'>";
      html += "<span class='metric-label'>Transceiver:</span>";
      html += "<span class='metric-value'>" + transceiver + "</span>";
      html += "</div>";
    }
    
    // Extract author
    int authorPos = modemFirmwareVersion.indexOf("(");
    int authorEnd = modemFirmwareVersion.indexOf(")", authorPos);
    if (authorPos != -1 && authorEnd != -1) {
      String author = modemFirmwareVersion.substring(authorPos + 1, authorEnd);
      html += "<div class='metric'>";
      html += "<span class='metric-label'>Author:</span>";
      html += "<span class='metric-value'>" + author + "</span>";
      html += "</div>";
    }
    
    // Extract Git ID
    int gitPos = modemFirmwareVersion.indexOf("GitID");
    if (gitPos != -1) {
      String gitId = modemFirmwareVersion.substring(gitPos + 6);
      gitId.trim();
      html += "<div class='metric'>";
      html += "<span class='metric-label'>Git ID:</span>";
      html += "<span class='metric-value'>" + gitId + "</span>";
      html += "</div>";
    }
  }
  html += "</div>";

  // System Control Buttons Card
  html += "<div class='card'>";
  html += "<h3>System Control</h3>";
  html += "<p>System control and management actions:</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='javascript:void(0)' onclick='rebootSystem()' class='btn btn-danger'>Reboot System</a>";
  html += "<a href='javascript:void(0)' onclick='restartServices()' class='btn btn-warning'>Restart Services</a>";
  html += "<a href='javascript:void(0)' onclick='clearLogs()' class='btn btn-info'>Clear Logs</a>";
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
  html += "function clearLogs() {";
  html += "  if (confirm('Clear all system logs?')) {";
  html += "    fetch('/clearlogs', {method: 'POST'}).then(() => {";
  html += "      alert('Logs cleared successfully!');";
  html += "    });";
  html += "  }";
  html += "}";
  html += "</script>";

  html += getAdminFooter();
  server.send(200, "text/html; charset=UTF-8", html);
}

#endif // WEB_PAGES_ADMIN_SYSTEM_H
