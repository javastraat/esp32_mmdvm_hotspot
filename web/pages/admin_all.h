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
  html += "<div class='metric'><span class='metric-label'>Firmware Version:</span><span class='metric-value'>" + firmwareVersion + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Device Name:</span><span class='metric-value'>" + dmr_callsign + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Uptime:</span><span class='metric-value'>" + String(uptimeDays) + "d " + String(displayHours) + "h " + String(displayMins) + "m " + String(displaySecs) + "s</span></div>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>ESP32 Hardware</h3>";
  html += "<div class='metric'><span class='metric-label'>Chip Model:</span><span class='metric-value'>" + String(ESP.getChipModel()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Free Heap:</span><span class='metric-value'>" + String(ESP.getFreeHeap() / 1024.0, 2) + " KB</span></div>";
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
  html += "<h3>Hostname</h3>";
  html += "<p>Current: <strong>" + device_hostname + "</strong></p>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>Debug Settings</h3>";
  html += "<p>Serial: " + String(debug_serial ? "✓" : "✗") + " | MMDVM: " + String(debug_mmdvm ? "✓" : "✗") + " | Network: " + String(debug_network ? "✓" : "✗") + "</p>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>OLED Display</h3>";
  html += "<p>Status: <strong>" + String(enable_oled ? "Enabled" : "Disabled") + "</strong></p>";
  html += "</div>";
  
  // Security Section
  html += "<div style='grid-column: 1 / -1;'>";
  html += "<h2 style='color: var(--primary-color); border-bottom: 2px solid var(--primary-color); padding-bottom: 10px; margin-top: 30px;'>Security</h2>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>Web Credentials</h3>";
  html += "<p>Username: <strong>" + web_username + "</strong></p>";
  html += "<p>Password: ********</p>";
  html += "</div>";
  
  // Network Section
  html += "<div style='grid-column: 1 / -1;'>";
  html += "<h2 style='color: var(--primary-color); border-bottom: 2px solid var(--primary-color); padding-bottom: 10px; margin-top: 30px;'>Network</h2>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>MQTT Configuration</h3>";
  html += "<p>Enabled: <strong>" + String(mqtt_enabled ? "Yes" : "No") + "</strong></p>";
  html += "<p>Broker: " + mqtt_broker + ":" + String(mqtt_port) + "</p>";
  html += "</div>";
  
  // Maintenance Section
  html += "<div style='grid-column: 1 / -1;'>";
  html += "<h2 style='color: var(--primary-color); border-bottom: 2px solid var(--primary-color); padding-bottom: 10px; margin-top: 30px;'>Maintenance</h2>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>Firmware</h3>";
  html += "<p>ESP32: <strong>" + firmwareVersion + "</strong></p>";
  html += "<p>Modem: <strong>" + modemFirmwareVersion + "</strong></p>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h3>Configuration</h3>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='/showprefs' class='btn btn-primary'>Show Preferences</a>";
  html += "<a href='javascript:void(0)' onclick='downloadConfig()' class='btn btn-success'>Export Config</a>";
  html += "</div>";
  html += "</div>";
  
  html += "</div>"; // Close admin-grid
  
  html += "<div class='info' style='background: var(--info-bg); color: var(--text-color); margin-top: 20px;'>";
  html += "<strong>Note:</strong> This is a simplified view showing all sections. Use individual section pages for full configuration options.";
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
  html += "</script>";
  
  html += getAdminFooter();
  server.send(200, "text/html; charset=UTF-8", html);
}

#endif // WEB_PAGES_ADMIN_ALL_H
