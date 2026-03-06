/*
 * system_info.h - System Information Page for ESP32 RTOS MMDVM Web Interface
 */

#ifndef WEB_SYSTEM_INFO_H
#define WEB_SYSTEM_INFO_H

#include <Arduino.h>
#include <ESP.h>
#include <WiFi.h>
#include <esp_system.h>
#include <esp_ota_ops.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"
#include "include/config.h"

// External variables (if you have modem firmware version tracking)
// extern String modemFirmwareVersion;


String getSystemInfoPageHTML()
{
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>System Info - ESP32 RTOS MMDVM</title>";
  html += getSharedStyles();
  html += "<style>";
  html += ".admin-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 15px; margin: 20px 0; }";
  html += ".metric { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #eee; }";
  html += ".metric:last-child { border-bottom: none; }";
  html += ".metric-label { font-weight: bold; color: #555; }";
  html += ".metric-value { color: #333; }";
  html += "</style></head><body>";
  html += getNavigation("system-info");
  html += "<div class='container'>";
  html += "<h1>System Information</h1>";
  html += "<p>Hardware details, software versions, and build information.</p>";
  html += "<div class='admin-grid'>";

  // ==================== System Hardware Card ====================
  html += "<div class='card'>";
  html += "<h3>System Hardware</h3>";

  html += "<div class='metric'><span class='metric-label'>Chip Model:</span><span class='metric-value'>" + String(ESP.getChipModel()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Chip Revision:</span><span class='metric-value'>" + String(ESP.getChipRevision()) + "</span></div>";

  // Chip ID from EFuse MAC
  uint64_t chipId = ESP.getEfuseMac();
  char chipIdStr[18];
  snprintf(chipIdStr, sizeof(chipIdStr), "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
  html += "<div class='metric'><span class='metric-label'>Chip ID:</span><span class='metric-value'>" + String(chipIdStr) + "</span></div>";

  html += "<div class='metric'><span class='metric-label'>CPU Cores:</span><span class='metric-value'>" + String(ESP.getChipCores()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>CPU Frequency:</span><span class='metric-value'>" + String(ESP.getCpuFreqMHz()) + " MHz</span></div>";

  // Internal temperature (ESP32-S3)
  float tempC = temperatureRead();
  if (tempC > -40 && tempC < 125)
  { // Valid range check
    html += "<div class='metric'><span class='metric-label'>CPU Temperature:</span><span class='metric-value'>" + String(tempC, 1) + " &deg;C</span></div>";
  }
  html += "</div>";

  // ==================== Memory Card ====================
  html += "<div class='card'>";
  html += "<h3>Memory</h3>";

  html += "<div class='metric'><span class='metric-label'>Heap Size:</span><span class='metric-value'>" + String(ESP.getHeapSize() / 1024.0, 1) + " KB</span></div>";
  html += "<div class='metric'><span class='metric-label'>Free Heap:</span><span class='metric-value'>" + String(ESP.getFreeHeap() / 1024.0, 1) + " KB (" + String(ESP.getFreeHeap() * 100 / ESP.getHeapSize()) + "%)</span></div>";
  html += "<div class='metric'><span class='metric-label'>Min Free Heap:</span><span class='metric-value'>" + String(ESP.getMinFreeHeap() / 1024.0, 1) + " KB</span></div>";
  html += "<div class='metric'><span class='metric-label'>Max Alloc Heap:</span><span class='metric-value'>" + String(ESP.getMaxAllocHeap() / 1024.0, 1) + " KB</span></div>";

  // PSRAM info (if available)
  if (ESP.getPsramSize() > 0)
  {
    html += "<div class='metric'><span class='metric-label'>PSRAM Size:</span><span class='metric-value'>" + String(ESP.getPsramSize() / 1024 / 1024) + " MB</span></div>";
    html += "<div class='metric'><span class='metric-label'>Free PSRAM:</span><span class='metric-value'>" + String(ESP.getFreePsram() / 1024.0, 1) + " KB</span></div>";
    html += "<div class='metric'><span class='metric-label'>Max Alloc PSRAM:</span><span class='metric-value'>" + String(ESP.getMaxAllocPsram() / 1024.0, 1) + " KB</span></div>";
  }
  html += "</div>";

  // ==================== Task Stack Usage Card ====================
  html += "<div class='card'>";
  html += "<h3>Task Stack Usage</h3>";

  {
    UBaseType_t taskCount = uxTaskGetNumberOfTasks();
    TaskStatus_t *taskArray = (TaskStatus_t *)pvPortMalloc(taskCount * sizeof(TaskStatus_t));
    if (taskArray != NULL) {
      UBaseType_t actualCount = uxTaskGetSystemState(taskArray, taskCount, NULL);

      // Collect matching tasks (name contains "Task" or equals "DMR Database")
      struct { const char *name; int freeBytes; } matched[32];
      int matchCount = 0;
      for (UBaseType_t i = 0; i < actualCount && matchCount < 32; i++) {
        const char *n = taskArray[i].pcTaskName;
        bool include = (strstr(n, " Task") != NULL) || (strcmp(n, "DMR Database") == 0);
        if (include) {
          matched[matchCount].name = n;
          matched[matchCount].freeBytes = (int)taskArray[i].usStackHighWaterMark * 4;
          matchCount++;
        }
      }

      // Sort by free bytes ascending (worst first)
      for (int a = 0; a < matchCount - 1; a++) {
        for (int b = a + 1; b < matchCount; b++) {
          if (matched[b].freeBytes < matched[a].freeBytes) {
            auto tmp = matched[a]; matched[a] = matched[b]; matched[b] = tmp;
          }
        }
      }

      for (int i = 0; i < matchCount; i++) {
        int fb = matched[i].freeBytes;
        String color = (fb < 512) ? "#f44336" : (fb < 1024) ? "#ff9800" : "#4CAF50";
        String freeStr = (fb >= 1024) ? String(fb / 1024.0, 1) + " KB" : String(fb) + " B";
        html += "<div class='metric'>";
        html += "<span class='metric-label'>" + String(matched[i].name) + ":</span>";
        html += "<span class='metric-value' style='color:" + color + "'>" + freeStr + " free</span>";
        html += "</div>";
      }

      vPortFree(taskArray);
    } else {
      html += "<p style='color:red;'>Memory allocation failed.</p>";
    }
  }

  html += "</div>";

  // ==================== All Tasks Card ====================
  html += "<div class='card'>";
  html += "<h3>All Tasks</h3>";

  UBaseType_t taskCount = uxTaskGetNumberOfTasks();
  TaskStatus_t *taskArray = (TaskStatus_t *)pvPortMalloc(taskCount * sizeof(TaskStatus_t));
  if (taskArray != NULL) {
    uint32_t totalRuntime = 0;
    UBaseType_t actualCount = uxTaskGetSystemState(taskArray, taskCount, &totalRuntime);

    html += "<details>";
    html += "<summary style='cursor:pointer;color:#007bff;font-size:0.9em;margin-bottom:8px;'>" + String(actualCount) + " tasks &mdash; click to expand</summary>";

    html += "<table id='tasks-table' style='width:100%;font-size:0.82em;'>";
    html += "<thead><tr>";
    html += "<th style='padding:3px 4px;cursor:pointer;' onclick='sortTaskTable(this,0,\"str\")'><span>Name</span><span class='si'></span></th>";
    html += "<th style='padding:3px 4px;text-align:center;cursor:pointer;' onclick='sortTaskTable(this,1,\"str\")'><span>State</span><span class='si'></span></th>";
    html += "<th style='padding:3px 4px;text-align:center;cursor:pointer;' onclick='sortTaskTable(this,2,\"num\")'><span>Pri</span><span class='si'></span></th>";
    html += "<th style='padding:3px 4px;text-align:center;cursor:pointer;' onclick='sortTaskTable(this,3,\"str\")'><span>Core</span><span class='si'></span></th>";
    html += "<th style='padding:3px 4px;text-align:right;cursor:pointer;' onclick='sortTaskTable(this,4,\"bytes\")'><span>Stack Free</span><span class='si'></span></th>";
    html += "</tr></thead>";
    html += "<tbody>";

    const char* stateNames[] = { "Run", "Ready", "Block", "Susp", "Del", "?" };

    for (UBaseType_t i = 0; i < actualCount; i++) {
      int si = (int)taskArray[i].eCurrentState;
      if (si < 0 || si > 5) si = 5;

      int freeBytes = (int)taskArray[i].usStackHighWaterMark * 4;
      String freeStr = (freeBytes >= 1024)
        ? String(freeBytes / 1024.0, 1) + " KB"
        : String(freeBytes) + " B";

      String stackColor = (freeBytes < 512) ? "#f44336" : (freeBytes < 1024) ? "#ff9800" : "";
      String stackStyle = stackColor.length() ? "color:" + stackColor + ";" : "";

      String coreStr = (taskArray[i].xCoreID == 0 || taskArray[i].xCoreID == 1)
        ? String(taskArray[i].xCoreID) : "*";

      html += "<tr>";
      html += "<td style='padding:3px 4px;'>" + String(taskArray[i].pcTaskName) + "</td>";
      html += "<td style='padding:3px 4px;text-align:center;'>" + String(stateNames[si]) + "</td>";
      html += "<td style='padding:3px 4px;text-align:center;'>" + String(taskArray[i].uxCurrentPriority) + "</td>";
      html += "<td style='padding:3px 4px;text-align:center;'>" + coreStr + "</td>";
      html += "<td style='padding:3px 4px;text-align:right;" + stackStyle + "'>" + freeStr + "</td>";
      html += "</tr>";
    }
    html += "</tbody></table>";
    html += "<p style='font-size:0.78em;color:var(--text-muted);margin-top:6px;'>* = any core. Red &lt;512B, orange &lt;1KB.</p>";
    html += "</details>";
    vPortFree(taskArray);
  } else {
    html += "<p style='color:red;'>Memory allocation failed.</p>";
  }
  html += "</div>";

  // ==================== Storage Card ====================
  html += "<div class='card'>";
  html += "<h3>Storage</h3>";

  html += "<div class='metric'><span class='metric-label'>Flash Size:</span><span class='metric-value'>" + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB</span></div>";
  html += "<div class='metric'><span class='metric-label'>Flash Speed:</span><span class='metric-value'>" + String(ESP.getFlashChipSpeed() / 1000000) + " MHz</span></div>";

  // Flash Mode
  const char *flashModes[] = {"QIO", "QOUT", "DIO", "DOUT", "Fast Read", "Slow Read"};
  FlashMode_t flashMode = ESP.getFlashChipMode();
  String flashModeStr = (flashMode < 6) ? flashModes[flashMode] : "Unknown";
  html += "<div class='metric'><span class='metric-label'>Flash Mode:</span><span class='metric-value'>" + flashModeStr + "</span></div>";

  html += "<div class='metric'><span class='metric-label'>Sketch Size:</span><span class='metric-value'>" + String(ESP.getSketchSize() / 1024.0, 1) + " KB</span></div>";
  html += "<div class='metric'><span class='metric-label'>Free Sketch Space:</span><span class='metric-value'>" + String(ESP.getFreeSketchSpace() / 1024.0, 1) + " KB</span></div>";
  String sketchMD5 = ESP.getSketchMD5();
  html += "<div class='metric'><span class='metric-label'>Sketch MD5:</span><span class='metric-value'>" + sketchMD5.substring(sketchMD5.length() - 8) + "</span></div>";

  // Running partition
  const esp_partition_t *partition = esp_ota_get_running_partition();
  if (partition != NULL)
  {
    html += "<div class='metric'><span class='metric-label'>Running Partition:</span><span class='metric-value'>" + String(partition->label) + "</span></div>";
  }
  html += "</div>";

  // ==================== Software Card ====================
  html += "<div class='card'>";
  html += "<h3>Software</h3>";

  // Uptime calculation
  unsigned long uptimeSeconds = millis() / 1000;
  unsigned long days = uptimeSeconds / 86400;
  unsigned long hours = (uptimeSeconds % 86400) / 3600;
  unsigned long minutes = (uptimeSeconds % 3600) / 60;
  unsigned long seconds = uptimeSeconds % 60;
  String uptimeStr = "";
  if (days > 0)
    uptimeStr += String(days) + "d ";
  if (hours > 0 || days > 0)
    uptimeStr += String(hours) + "h ";
  uptimeStr += String(minutes) + "m " + String(seconds) + "s";

  html += "<div class='metric'><span class='metric-label'>Uptime:</span><span class='metric-value'>" + uptimeStr + "</span></div>";

  // Reset reason
  esp_reset_reason_t resetReason = esp_reset_reason();
  const char *resetReasons[] = {"Unknown", "Power-on", "External", "Software", "Panic", "Interrupt WDT", "Task WDT", "Other WDT", "Deepsleep", "Brownout", "SDIO"};
  String resetReasonStr = (resetReason < 11) ? resetReasons[resetReason] : "Unknown";
  html += "<div class='metric'><span class='metric-label'>Reset Reason:</span><span class='metric-value'>" + resetReasonStr + "</span></div>";

  html += "<div class='metric'><span class='metric-label'>SDK Version:</span><span class='metric-value'>" + String(ESP.getSdkVersion()) + "</span></div>";
#ifdef ESP_ARDUINO_VERSION_STR
  html += "<div class='metric'><span class='metric-label'>Arduino Core:</span><span class='metric-value'>" + String(ESP_ARDUINO_VERSION_STR) + "</span></div>";
#endif

  // If you have FIRMWARE_VERSION defined in config.h, uncomment this:
  // html += "<div class='metric'><span class='metric-label'>Firmware Version:</span><span class='metric-value'>" + String(FIRMWARE_VERSION) + "</span></div>";

  html += "<div class='metric'><span class='metric-label'>Build Date:</span><span class='metric-value'>" + String(__DATE__) + " " + String(__TIME__) + "</span></div>";
  html += "</div>";

  // ==================== Modem Information Card ====================
  // Uncomment when you have modemFirmwareVersion available
  /*
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
  */

  // ==================== Network Card ====================
  // html += "<div class='card'>";
  // html += "<h3>Network</h3>";
  // html += "<div class='metric'><span class='metric-label'>MAC Address:</span><span class='metric-value'>" + WiFi.macAddress() + "</span></div>";
  // if (WiFi.status() == WL_CONNECTED)
  // {
  //   html += "<div class='metric'><span class='metric-label'>IP Address:</span><span class='metric-value'>" + WiFi.localIP().toString() + "</span></div>";
  //   html += "<div class='metric'><span class='metric-label'>SSID:</span><span class='metric-value'>" + WiFi.SSID() + "</span></div>";
  //   html += "<div class='metric'><span class='metric-label'>RSSI:</span><span class='metric-value'>" + String(WiFi.RSSI()) + " dBm</span></div>";
  // }
  // html += "</div>";

  html += "</div>"; // Close admin-grid

  html += getFooter();
  html += "</div>"; // Close container

  html += "<script>";
  html += "var _tsd={};";
  html += "function sortTaskTable(th,col,type){";
  html += "  var dir=_tsd[col]==='asc'?'desc':'asc';";
  html += "  _tsd={};_tsd[col]=dir;";
  html += "  var tbody=document.getElementById('tasks-table').querySelector('tbody');";
  html += "  var rows=Array.from(tbody.querySelectorAll('tr'));";
  html += "  function toB(s){var m=s.match(/([\\d.]+)\\s*(KB|B)/);if(!m)return 0;return m[2]==='KB'?parseFloat(m[1])*1024:parseFloat(m[1]);}";
  html += "  rows.sort(function(a,b){";
  html += "    var av=a.cells[col].textContent.trim();";
  html += "    var bv=b.cells[col].textContent.trim();";
  html += "    if(type==='num'){av=parseFloat(av)||0;bv=parseFloat(bv)||0;}";
  html += "    else if(type==='bytes'){av=toB(av);bv=toB(bv);}";
  html += "    if(av<bv)return dir==='asc'?-1:1;";
  html += "    if(av>bv)return dir==='asc'?1:-1;";
  html += "    return 0;";
  html += "  });";
  html += "  rows.forEach(function(r){tbody.appendChild(r);});";
  html += "  document.getElementById('tasks-table').querySelectorAll('th .si').forEach(function(s,i){s.textContent=i===col?(dir==='asc'?' \u25b2':' \u25bc'):'';});";
  html += "}";
  html += "</script>";

  html += "</body></html>";

  return html;
}

#endif // WEB_SYSTEM_INFO_H
