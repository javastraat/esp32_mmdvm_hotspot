/*
 * admin_all.h - Admin "All Sections" Page
 * Shows all admin sections combined in one view for debugging
 */

#ifndef WEB_PAGES_ADMIN_ALL_H
#define WEB_PAGES_ADMIN_ALL_H

#include <Arduino.h>

// Include all admin section handlers
void handleAdminSystem();
void handleAdminSettings();
void handleAdminSecurity();
void handleAdminNetwork();
void handleAdminMaintenance();

void handleAdminAll() {
  if (!checkAuthentication()) return;

  // This is a special case - we generate a combined page showing all sections
  // We can't just call each handler since they each send their own response
  // Instead, we build one large combined HTML page
  
  String html;
  html.reserve(50000);  // Large reserve for all sections combined
  
  html = getAdminHeader("All Admin Sections", "admin");
  
  html += "<p style='color: var(--text-secondary); margin-bottom: 20px;'>Complete overview of all administration settings for debugging and review</p>";
  
  html += "<div class='admin-grid'>";
  
  // System Section
  html += "<div style='grid-column: 1 / -1;'>";
  html += "<h2 style='color: var(--primary-color); border-bottom: 2px solid var(--primary-color); padding-bottom: 10px; margin-top: 30px;'>System</h2>";
  html += "</div>";
  
  // Calculate uptime
  unsigned long uptimeMillis = millis();
  unsigned long uptimeSecs = uptimeMillis / 1000;
  unsigned long uptimeMins = uptimeSecs / 60;
  unsigned long uptimeHours = uptimeMins / 60;
  unsigned long uptimeDays = uptimeHours / 24;
  unsigned long displayHours = uptimeHours % 24;
  unsigned long displayMins = uptimeMins % 60;
  unsigned long displaySecs = uptimeSecs % 60;
  
  html += "<div class='card'>";
  html += "<h3>System Information</h3>";
  html += "<div class='metric'><span class='metric-label'>Uptime:</span><span class='metric-value'>" + String(uptimeDays) + "d " + String(displayHours) + "h " + String(displayMins) + "m " + String(displaySecs) + "s</span></div>";
  html += "<div class='metric'><span class='metric-label'>Chip Model:</span><span class='metric-value'>" + String(ESP.getChipModel()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Chip Revision:</span><span class='metric-value'>" + String(ESP.getChipRevision()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>CPU Cores:</span><span class='metric-value'>" + String(ESP.getChipCores()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>CPU Frequency:</span><span class='metric-value'>" + String(ESP.getCpuFreqMHz()) + " MHz</span></div>";
  html += "<div class='metric'><span class='metric-label'>Free Heap:</span><span class='metric-value'>" + String(ESP.getFreeHeap() / 1024.0, 1) + " KB (" + String(ESP.getFreeHeap() * 100 / ESP.getHeapSize()) + "%)</span></div>";
  html += "<div class='metric'><span class='metric-label'>Min Free Heap:</span><span class='metric-value'>" + String(ESP.getMinFreeHeap() / 1024.0, 1) + " KB</span></div>";
  html += "<div class='metric'><span class='metric-label'>Heap Size:</span><span class='metric-value'>" + String(ESP.getHeapSize() / 1024.0, 1) + " KB</span></div>";
  if (ESP.getPsramSize() > 0) {
    html += "<div class='metric'><span class='metric-label'>PSRAM Size:</span><span class='metric-value'>" + String(ESP.getPsramSize() / 1024 / 1024) + " MB</span></div>";
    html += "<div class='metric'><span class='metric-label'>Free PSRAM:</span><span class='metric-value'>" + String(ESP.getFreePsram() / 1024.0, 1) + " KB</span></div>";
  }
  html += "<div class='metric'><span class='metric-label'>Flash Size:</span><span class='metric-value'>" + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB</span></div>";
  html += "<div class='metric'><span class='metric-label'>Flash Speed:</span><span class='metric-value'>" + String(ESP.getFlashChipSpeed() / 1000000) + " MHz</span></div>";
  html += "<div class='metric'><span class='metric-label'>Sketch Size:</span><span class='metric-value'>" + String(ESP.getSketchSize() / 1024.0, 1) + " KB</span></div>";
  html += "<div class='metric'><span class='metric-label'>Free Sketch Space:</span><span class='metric-value'>" + String(ESP.getFreeSketchSpace() / 1024.0, 1) + " KB</span></div>";
  html += "<div class='metric'><span class='metric-label'>SDK Version:</span><span class='metric-value'>" + String(ESP.getSdkVersion()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Firmware Version:</span><span class='metric-value'>" + String(FIRMWARE_VERSION) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Build Date:</span><span class='metric-value'>" + String(__DATE__) + " " + String(__TIME__) + "</span></div>";
  html += "</div>";
  
  // Modem Information Card
  html += "<div class='card'>";
  html += "<h3>Modem Information</h3>";
  
  String hardware = "Unknown";
  String version = "Unknown";
  String buildDate = "Unknown";
  String crystal = "Unknown";
  String transceiver = "Unknown";
  String author = "Unknown";
  String gitId = "Unknown";

  if (modemFirmwareVersion != "Unknown" && modemFirmwareVersion.length() > 0) {
    String fwStr = modemFirmwareVersion;
    int vPos = fwStr.indexOf("-v");
    if (vPos > 0) {
      hardware = fwStr.substring(0, vPos);
      fwStr = fwStr.substring(vPos + 2);
    } else {
      int spacePos = fwStr.indexOf(' ');
      if (spacePos > 0) {
        hardware = fwStr.substring(0, spacePos);
        fwStr = fwStr.substring(spacePos + 1);
      }
    }
    int spacePos = fwStr.indexOf(' ');
    if (spacePos > 0) {
      version = fwStr.substring(0, spacePos);
      fwStr = fwStr.substring(spacePos + 1);
    }
    spacePos = fwStr.indexOf(' ');
    if (spacePos > 0) {
      String dateStr = fwStr.substring(0, spacePos);
      if (dateStr.length() >= 8) {
        bool isValidDate = true;
        for (int i = 0; i < 8; i++) {
          if (!isdigit(dateStr.charAt(i))) {
            isValidDate = false;
            break;
          }
        }
        if (isValidDate) {
          buildDate = dateStr.substring(6, 8) + "-" + dateStr.substring(4, 6) + "-" + dateStr.substring(0, 4);
        }
      }
      fwStr = fwStr.substring(spacePos + 1);
    }
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
    fwStr.trim();
    int fwPos = fwStr.indexOf(" FW");
    if (fwPos > 0) {
      spacePos = fwStr.indexOf(' ');
      if (spacePos > 0) {
        transceiver = fwStr.substring(0, spacePos);
      } else {
        transceiver = fwStr.substring(0, fwPos);
      }
      fwStr = fwStr.substring(fwPos + 3);
    }
    int byPos = fwStr.indexOf("by ");
    int gitPos = fwStr.indexOf(" GitID");
    if (byPos >= 0 && gitPos > byPos) {
      author = fwStr.substring(byPos + 3, gitPos);
      author.trim();
    }
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
  
  html += "<div class='card'>";
  html += "<h3>System Control</h3>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='javascript:void(0)' onclick='rebootSystem()' class='btn btn-danger'>Reboot System</a>";
  html += "<a href='javascript:void(0)' onclick='restartServices()' class='btn btn-warning'>Restart Services</a>";
  html += "</div>";
  html += "</div>";
  
  // Settings Section
  html += "<div style='grid-column: 1 / -1;'>";
  html += "<h2 style='color: var(--primary-color); border-bottom: 2px solid var(--primary-color); padding-bottom: 10px; margin-top: 30px;'>Settings</h2>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>Hostname Configuration</h3>";
  html += "<p>Current hostname: <strong>" + device_hostname + "</strong></p>";
  html += "<p style='font-size:0.9em;color:var(--text-color);'>Access via: http://" + device_hostname + ".local</p>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>Debug Settings</h3>";
  html += "<p><strong>Serial Output:</strong> " + String(debug_serial ? "✓ Enabled" : "✗ Disabled") + "</p>";
  html += "<p><strong>Verbose Logging:</strong> " + String(verbose_logging ? "✓ Enabled" : "✗ Disabled") + "</p>";
  html += "<p><strong>MMDVM Protocol:</strong> " + String(debug_mmdvm ? "✓ Enabled" : "✗ Disabled") + "</p>";
  html += "<p><strong>Network Debug:</strong> " + String(debug_network ? "✓ Enabled" : "✗ Disabled") + "</p>";
  html += "<p><strong>DMR Protocol:</strong> " + String(debug_dmr ? "✓ Enabled" : "✗ Disabled") + "</p>";
  html += "<p><strong>Password Debug:</strong> " + String(debug_password ? "✓ Enabled" : "✗ Disabled") + "</p>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>OLED Display Settings</h3>";
  html += "<p>Status: <strong>" + String(enable_oled ? "Enabled" : "Disabled") + "</strong></p>";
  html += "<p><strong>Auto-Blanking:</strong> " + String(oledAutoBlankEnabled ? "Enabled" : "Disabled") + "</p>";
  if (oledAutoBlankEnabled && oledBlankTimeout > 0) {
    html += "<p><strong>Blank Timeout:</strong> ";
    if (oledBlankTimeout == 30000) html += "30 seconds";
    else if (oledBlankTimeout == 60000) html += "1 minute";
    else if (oledBlankTimeout == 120000) html += "2 minutes";
    else if (oledBlankTimeout == 300000) html += "5 minutes";
    else if (oledBlankTimeout == 600000) html += "10 minutes";
    else html += String(oledBlankTimeout / 1000) + " seconds";
    html += "</p>";
  }
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>NTP Timezone Configuration</h3>";
  html += "<p>Timezone offset: <strong>" + String(ntp_timezone_offset / 3600.0, 1) + " hours</strong></p>";
  html += "<p>DST offset: <strong>" + String(ntp_daylight_offset / 3600.0, 1) + " hours</strong></p>";
  html += "</div>";
  
  // Security Section
  html += "<div style='grid-column: 1 / -1;'>";
  html += "<h2 style='color: var(--primary-color); border-bottom: 2px solid var(--primary-color); padding-bottom: 10px; margin-top: 30px;'>Security</h2>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>Web Username</h3>";
  html += "<p>Current Username: <strong>" + web_username + "</strong></p>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>Web Password</h3>";
  html += "<p>Current Password: <strong>********</strong></p>";
  html += "<p style='font-size:0.9em;color:var(--text-color);'>Password is hidden for security</p>";
  html += "</div>";
  
  // Network Section
  html += "<div style='grid-column: 1 / -1;'>";
  html += "<h2 style='color: var(--primary-color); border-bottom: 2px solid var(--primary-color); padding-bottom: 10px; margin-top: 30px;'>Network</h2>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>MQTT Configuration</h3>";
  html += "<p><strong>Enabled:</strong> " + String(mqtt_enabled ? "Yes" : "No") + "</p>";
  html += "<p><strong>Broker:</strong> " + mqtt_broker + "</p>";
  html += "<p><strong>Port:</strong> " + String(mqtt_port) + "</p>";
  html += "<p><strong>Username:</strong> " + (mqtt_username.length() > 0 ? mqtt_username : "(none)") + "</p>";
  html += "<p><strong>Client ID:</strong> " + mqtt_client_id + "</p>";
  html += "<p><strong>Topic Prefix:</strong> " + mqtt_topic_prefix + "</p>";
  html += "<p><strong>Publish Interval:</strong> " + String(mqtt_publish_interval / 1000) + " seconds</p>";
  html += "</div>";
  
  // Maintenance Section
  html += "<div style='grid-column: 1 / -1;'>";
  html += "<h2 style='color: var(--primary-color); border-bottom: 2px solid var(--primary-color); padding-bottom: 10px; margin-top: 30px;'>Maintenance</h2>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>ESP32 Firmware</h3>";
  html += "<p><strong>Current Version:</strong> " + firmwareVersion + "</p>";
  html += "<p><strong>Build Date:</strong> " + String(__DATE__) + " " + String(__TIME__) + "</p>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>MMDVM Modem Firmware</h3>";
  html += "<p><strong>Current Version:</strong> " + modemFirmwareVersion + "</p>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>Configuration Management</h3>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='/showprefs' class='btn btn-primary'>Show Preferences</a>";
  html += "<a href='javascript:void(0)' onclick='downloadConfig()' class='btn btn-success'>Export Config</a>";
  html += "<a href='/resetconfig' class='btn btn-danger'>Reset All Settings</a>";
  html += "</div>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>Maintenance Tools</h3>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='javascript:void(0)' onclick='clearLogs()' class='btn btn-warning'>Clear Logs</a>";
  html += "<a href='javascript:void(0)' onclick='cleanupPrefs()' class='btn btn-danger'>Fix Corrupted Prefs</a>";
  html += "</div>";
  html += "</div>";
  
  html += "</div>"; // Close admin-grid
  
  html += "<div class='info' style='background: var(--info-bg); color: var(--text-color); margin-top: 20px;'>";
  html += "<strong>Note:</strong> This page shows all administration settings in one view for quick reference. ";
  html += "Use individual section pages for interactive configuration and full details.";
  html += "</div>";
  
  // Minimal JavaScript
  html += "<script>";
  html += "function rebootSystem() {";
  html += "  if (confirm('Reboot system?')) {";
  html += "    fetch('/reboot', {method: 'POST'}).then(() => {";
  html += "      alert('Rebooting... Please wait 30 seconds.');";
  html += "      setTimeout(() => { window.location.href = '/'; }, 30000);";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function restartServices() {";
  html += "  if (confirm('Restart services?')) {";
  html += "    fetch('/restart-services', {method: 'POST'}).then(() => {";
  html += "      alert('Services restarted!');";
  html += "      setTimeout(() => { location.reload(); }, 2000);";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function downloadConfig() {";
  html += "  fetch('/export-config').then(r => r.blob()).then(blob => {";
  html += "    const url = window.URL.createObjectURL(blob);";
  html += "    const a = document.createElement('a');";
  html += "    a.href = url;";
  html += "    a.download = 'mmdvm-config.txt';";
  html += "    a.click();";
  html += "    window.URL.revokeObjectURL(url);";
  html += "  });";
  html += "}";
  html += "function clearLogs() {";
  html += "  if (confirm('Clear all system logs?')) {";
  html += "    fetch('/clearlogs', {method: 'POST'}).then(() => {";
  html += "      alert('Logs cleared successfully!');";
  html += "    });";
  html += "  }";
  html += "}";
  html += "function cleanupPrefs() {";
  html += "  if (confirm('Fix Corrupted Prefs?\\n\\nThis will repair any corrupted or missing preference values.')) {";
  html += "    fetch('/cleanup-prefs', {method: 'POST'}).then(response => response.text()).then(data => {";
  html += "      alert(data);";
  html += "    });";
  html += "  }";
  html += "}";
  html += "</script>";
  
  html += getAdminFooter();
  server.send(200, "text/html; charset=UTF-8", html);
}

#endif // WEB_PAGES_ADMIN_ALL_H
