/*
 * monitor.h - Serial Monitor Page for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_PAGES_MONITOR_H
#define WEB_PAGES_MONITOR_H

#include <Arduino.h>
#include <WebServer.h>
#include "../common/css.h"
#include "../common/navigation.h"
#include "../common/utils.h"

// External variables
extern WebServer server;
extern String dmr_callsign;
extern String serialLog[];
extern int serialLogIndex;
#define SERIAL_LOG_SIZE 50

void handleMonitor() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>" + dmr_callsign + " - ESP32 MMDVM Hotspot</title>";
  html += getCommonCSS();
  html += "<style>";
  html += "#logs { background: #0e0e0e; color: #d4d4d4; font-family: 'Courier New', monospace; padding: 15px; border: 1px solid var(--border-color); border-radius: 4px; min-height: 400px; max-height: 600px; overflow-y: auto; }";
  html += ".log-line { margin: 3px 0; color: #cccccc; font-size: 13px; }";
  html += ".refresh-info { color: var(--text-color); font-size: 0.9em; margin: 10px 0; padding: 8px; background: var(--info-bg); border-radius: 4px; }";
  html += ".controls { margin: 15px 0; padding: 10px; background: var(--card-bg); border-radius: 4px; }";
  html += ".btn { padding: 8px 16px; margin: 5px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer; }";
  html += ".btn:hover { background: #0056b3; }";
  html += ".btn.danger { background: #dc3545; }";
  html += ".btn.danger:hover { background: #c82333; }";
  html += "</style>";
  html += "<script>";
  html += "let autoRefresh = true;";
  html += "function updateLogs() {";
  html += "  if (!autoRefresh) return;";
  html += "  fetch('/logs').then(r => r.text()).then(data => {";
  html += "    document.getElementById('logs').innerHTML = data;";
  html += "    document.getElementById('logs').scrollTop = document.getElementById('logs').scrollHeight;";
  html += "  }).catch(e => console.log('Failed to fetch logs:', e));";
  html += "}";
  html += "function toggleAutoRefresh() {";
  html += "  autoRefresh = !autoRefresh;";
  html += "  document.getElementById('toggleBtn').textContent = autoRefresh ? 'Pause' : 'Resume';";
  html += "  document.getElementById('status').textContent = autoRefresh ? 'Auto-refreshing every 2 seconds...' : 'Auto-refresh paused';";
  html += "}";
  html += "function clearLogs() {";
  html += "  if (confirm('Clear all logs?')) {";
  html += "    fetch('/clearlogs', {method: 'POST'}).then(() => updateLogs());";
  html += "  }";
  html += "}";
  html += "function toggleNav() {";
  html += "  var x = document.getElementById('myTopnav');";
  html += "  if (x.className === 'topnav') {";
  html += "    x.className += ' responsive';";
  html += "  } else {";
  html += "    x.className = 'topnav';";
  html += "  }";
  html += "}";
  html += "setInterval(updateLogs, 2000);";
  html += "window.onload = updateLogs;";
  html += "</script>";
  html += "</head><body>";
  html += getNavigation("monitor");
  html += "<div class='container'>";
  html += "<h1>Serial Monitor</h1>";
  html += "<div class='refresh-info' id='status'>Auto-refreshing every 2 seconds...</div>";
  html += "<div class='controls'>";
  html += "<button class='btn' onclick='updateLogs()'>Refresh Now</button>";
  html += "<button class='btn' id='toggleBtn' onclick='toggleAutoRefresh()'>Pause</button>";
  html += "<button class='btn danger' onclick='clearLogs()'>Clear Logs</button>";
  html += "</div>";
  html += "<div id='logs'>Loading...</div>";
  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleGetLogs() {
  if (!checkAuthentication()) return;

  String logs = "";

  // Display logs in order (oldest first)
  int start = serialLogIndex;
  for (int i = 0; i < SERIAL_LOG_SIZE; i++) {
    int idx = (start + i) % SERIAL_LOG_SIZE;
    if (serialLog[idx].length() > 0) {
      logs += "<div class='log-line'>" + serialLog[idx] + "</div>";
    }
  }

  if (logs.length() == 0) {
    logs = "<div class='log-line'>No logs yet...</div>";
  }

  server.send(200, "text/html", logs);
}

void handleClearLogs() {
  // Clear the serial log array
  for (int i = 0; i < SERIAL_LOG_SIZE; i++) {
    serialLog[i] = "";
  }
  serialLogIndex = 0;
  extern void logSerial(String message);
  logSerial("Logs cleared by user");
  server.send(200, "text/plain", "Logs cleared");
}

#endif // WEB_PAGES_MONITOR_H
