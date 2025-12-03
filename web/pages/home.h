/*
 * home.h - Home Page for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_PAGES_HOME_H
#define WEB_PAGES_HOME_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "../common/css.h"
#include "../common/navigation.h"
#include "../common/utils.h"

// External variables
extern WebServer server;
extern bool wifiConnected;
extern bool apMode;
extern bool mmdvmReady;
extern bool dmrLoggedIn;
extern uint32_t currentTalkgroup;
extern String dmrLoginStatus;
extern String dmr_callsign;
extern String dmr_server;
extern uint32_t dmr_rx_freq;
extern uint32_t dmr_tx_freq;
extern uint8_t dmr_color_code;

// DMR Activity structure
struct DMRActivity {
  uint32_t srcId;
  uint32_t dstId;
  uint8_t slotNo;
  bool isGroup;
  String frameType;
  String srcCallsign;
  String srcName;
  String srcCity;
  String srcCountry;
  unsigned long lastUpdate;
  unsigned long startTime;  // Actual transmission start time
  bool active;
};
extern DMRActivity dmrActivity[2];

// DMR History structure (matches esp32_mmdvm_hotspot.ino)
struct DMRHistory {
  String timestamp;
  uint32_t srcId;
  String srcCallsign;
  String srcName;
  String srcLocation;
  uint32_t dstId;
  bool isGroup;
  uint32_t duration;
  uint8_t ber;
  uint8_t rssi;
  uint8_t slotNo;
};
extern DMRHistory dmrHistory[15];
extern int dmrHistoryIndex;

// DMR Callsign lookup function
extern String lookupCallsign(uint32_t dmrId);

// Helper function to generate DMR activity HTML
String getDMRActivityHTML() {
  String html = "<div class='activity-grid'>";
  
  bool anyActivity = false;
  
  // Check both slots
  for (int i = 0; i < 2; i++) {
    DMRActivity &activity = dmrActivity[i];
    
    String cardClass = activity.active ? "activity-card" : "activity-idle";
    html += "<div class='" + cardClass + "'>";
    html += "<div class='slot-header'>Slot " + String(activity.slotNo) + "</div>";
    html += "<div class='activity-details'>";
    
    if (activity.active) {
      anyActivity = true;
      html += "<div class='activity-row'>";
      html += "<span class='label'>Station:</span>";
      html += "<span class='value'>";
      if (activity.srcCallsign.length() > 0) {
        html += activity.srcCallsign + " (" + String(activity.srcId) + ")";
        if (activity.srcName.length() > 0) {
          html += "<br><small>" + activity.srcName + "</small>";
        }
        if (activity.srcCity.length() > 0 || activity.srcCountry.length() > 0) {
          html += "<br><small>";
          if (activity.srcCity.length() > 0) html += activity.srcCity;
          if (activity.srcCity.length() > 0 && activity.srcCountry.length() > 0) html += ", ";
          if (activity.srcCountry.length() > 0) html += activity.srcCountry;
          html += "</small>";
        }
      } else {
        html += String(activity.srcId);
      }
      html += "</span>";
      html += "</div>";
      
      html += "<div class='activity-row'>";
      html += "<span class='label'>Destination:</span>";
      html += "<span class='value'>";
      if (activity.isGroup) html += "TG ";
      html += String(activity.dstId) + "</span>";
      html += "</div>";
      
      html += "<div class='activity-row'>";
      html += "<span class='label'>Type:</span>";
      html += "<span class='value'>" + activity.frameType + "</span>";
      html += "</div>";
      
      // Calculate transmission duration from start time
      unsigned long duration = (millis() - activity.startTime) / 1000;
      html += "<div class='activity-row'>";
      html += "<span class='label'>Duration:</span>";
      html += "<span class='value'>" + String(duration) + "s</span>";
      html += "</div>";
    } else {
      html += "<div class='no-activity'>No Active Transmission</div>";
    }
    
    html += "</div>";
    html += "</div>";
  }
  
  html += "</div>";
  
  return html;
}

void handleDMRActivity() {
  server.send(200, "text/html", getDMRActivityHTML());
}

// Helper function to generate DMR History HTML
String getDMRHistoryHTML() {
  String html = "<div class='history-container'>";
  
  // Count actual history entries
  int entryCount = 0;
  for (int i = 0; i < 15; i++) {
    if (dmrHistory[i].srcId > 0) entryCount++;
  }
  
  if (entryCount == 0) {
    html += "<div class='no-history'>No recent transmissions</div>";
  } else {
    html += "<div class='history-header'>";
    html += "<div class='col-time'>Time</div>";
    html += "<div class='col-station'>Station</div>";
    html += "<div class='col-destination'>Destination</div>";
    html += "<div class='col-duration'>Duration</div>";
    html += "<div class='col-slot'>Slot</div>";
    html += "</div>";
    
    // Show entries in reverse chronological order (newest first)
    for (int i = 0; i < 15; i++) {
      int index = (dmrHistoryIndex - 1 - i + 15) % 15;
      if (dmrHistory[index].srcId > 0) {
        html += "<div class='history-row'>";
        
        // Time
        html += "<div class='col-time'>" + dmrHistory[index].timestamp + "</div>";
        
        // Station info
        html += "<div class='col-station'>";
        if (dmrHistory[index].srcCallsign.length() > 0) {
          html += "<div class='callsign'>" + dmrHistory[index].srcCallsign + "</div>";
          if (dmrHistory[index].srcName.length() > 0) {
            html += "<div class='name'>" + dmrHistory[index].srcName + "</div>";
          }
          if (dmrHistory[index].srcLocation.length() > 0) {
            html += "<div class='location'>" + dmrHistory[index].srcLocation + "</div>";
          }
        } else {
          html += "<div class='callsign'>" + String(dmrHistory[index].srcId) + "</div>";
        }
        html += "</div>";
        
        // Destination
        html += "<div class='col-destination'>";
        if (dmrHistory[index].isGroup) html += "TG ";
        html += String(dmrHistory[index].dstId) + "</div>";
        
        // Duration
        html += "<div class='col-duration'>" + String(dmrHistory[index].duration) + "s</div>";
        
        // Slot
        html += "<div class='col-slot'>" + String(dmrHistory[index].slotNo) + "</div>";
        
        html += "</div>";
      }
    }
  }
  
  html += "</div>";
  return html;
}

void handleDMRHistory() {
  server.send(200, "text/html", getDMRHistoryHTML());
}

void handleRoot() {
  if (!checkAuthentication()) return;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>" + dmr_callsign + " - ESP32 MMDVM Hotspot</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".activity-card { border-left: 4px solid #4CAF50; animation: pulse 2s infinite; }";
  html += ".activity-idle { border-left: 4px solid #999; opacity: 0.6; }";
  html += "@keyframes pulse { 0%, 100% { border-color: #4CAF50; } 50% { border-color: #81C784; } }";
  html += ".activity-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 15px; }";
  html += ".slot-header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 10px; border-radius: 5px 5px 0 0; font-weight: bold; }";
  html += ".activity-details { padding: 15px; background: #f8f9fa; }";
  html += ".activity-row { display: flex; justify-content: space-between; padding: 5px 0; border-bottom: 1px solid #e0e0e0; }";
  html += ".activity-row:last-child { border-bottom: none; }";
  html += ".label { font-weight: bold; color: #555; }";
  html += ".value { color: #333; font-family: monospace; }";
  html += ".no-activity { text-align: center; padding: 30px; color: #999; font-style: italic; }";
  // History card styles
  html += ".history-card { margin-top: 20px; }";
  html += ".history-container { overflow-x: auto; }";
  html += ".history-header { display: grid; grid-template-columns: 80px 1fr 120px 80px 50px; gap: 10px; padding: 10px; background: #e9ecef; font-weight: bold; border-radius: 5px; margin-bottom: 5px; }";
  html += ".history-row { display: grid; grid-template-columns: 80px 1fr 120px 80px 50px; gap: 10px; padding: 8px 10px; border-bottom: 1px solid #e0e0e0; }";
  html += ".history-row:nth-child(even) { background: #f8f9fa; }";
  html += ".history-row:hover { background: #e3f2fd; }";
  html += ".col-time { font-family: monospace; font-size: 0.9em; }";
  html += ".col-station .callsign { font-weight: bold; color: #2196F3; }";
  html += ".col-station .name { font-size: 0.85em; color: #666; }";
  html += ".col-station .location { font-size: 0.8em; color: #999; }";
  html += ".col-destination { font-family: monospace; font-weight: bold; }";
  html += ".col-duration { text-align: center; font-family: monospace; }";
  html += ".col-slot { text-align: center; font-weight: bold; }";
  html += ".no-history { text-align: center; padding: 40px; color: #999; font-style: italic; }";
  html += "@media (max-width: 768px) { .history-header, .history-row { grid-template-columns: 60px 1fr 80px 50px; } .col-slot { display: none; } }";
  html += "</style>";
  html += "<script>";
  html += "function refreshActivity() {";
  html += "  fetch('/dmr-activity').then(r => r.text()).then(data => {";
  html += "    document.getElementById('dmr-activity-content').innerHTML = data;";
  html += "  });";
  html += "}";
  html += "function refreshHistory() {";
  html += "  fetch('/dmr-history').then(r => r.text()).then(data => {";
  html += "    document.getElementById('dmr-history-content').innerHTML = data;";
  html += "  });";
  html += "}";
  html += "setInterval(refreshActivity, 1000);";
  html += "setInterval(refreshHistory, 2000);";
  html += "</script>";
  html += "</head><body>";
  html += getNavigation("main");
  html += "<div class='container'>";
  html += "<h1><center>" + dmr_callsign + " - ESP32 MMDVM Hotspot</center></h1>";

  // Live DMR Activity Section (moved to top)
  html += "<div class='card'>";
  html += "<h3>Live DMR Activity</h3>";
  html += "<div id='dmr-activity-content'>";
  html += getDMRActivityHTML();
  html += "</div>";
  html += "</div>";

  // Recent Activity History Card (full width)
  html += "<div class='card history-card'>";
  html += "<h3>Recent DMR Activity (Last 15 Transmissions)</h3>";
  html += "<div id='dmr-history-content'>";
  html += getDMRHistoryHTML();
  html += "</div>";
  html += "</div>";

  html += "<div class='grid'>";
  html += "<div class='card'>";
  html += "<h3>Quick Status</h3>";

  if (wifiConnected) {
    html += "<div class='status connected'>WiFi Connected</div>";
    html += "<div class='info'>IP: " + WiFi.localIP().toString() + "</div>";
  } else if (apMode) {
    html += "<div class='status warning'>Access Point Mode</div>";
    html += "<div class='info'>AP IP: " + WiFi.softAPIP().toString() + "</div>";
  } else {
    html += "<div class='status disconnected'>WiFi Disconnected</div>";
  }

  String bmStatusClass = dmrLoggedIn ? "connected" : "disconnected";
  html += "<div class='status " + bmStatusClass + ">DMR: " + dmrLoginStatus + "</div>";

  String mmdvmIcon = mmdvmReady ? "[OK]" : "[ERR]";
  String mmdvmClass = mmdvmReady ? "connected" : "disconnected";
  html += "<div class='status " + mmdvmClass + ">" + mmdvmIcon + " MMDVM: " + (mmdvmReady ? "Ready" : "Not Ready") + "</div>";
  html += "</div>";
  html += "</div>";

  html += "<h2>About</h2>";
  html += "<p>Welcome to the ESP32 MMDVM Hotspot web interface. Use the navigation menu above to access different sections:</p>";
  html += "<ul>";
  html += "<li><strong>Status:</strong> Detailed system status and logs</li>";
  html += "<li><strong>Serial Monitor:</strong> Real-time MMDVM communication logs</li>";
  html += "<li><strong>WiFi Config:</strong> Configure alternate WiFi networks</li>";
  html += "<li><strong>Mode Config:</strong> Configure DMR and other mode settings</li>";
  html += "<li><strong>Admin:</strong> System administration and maintenance</li>";
  html += "</ul>";

  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

#endif // WEB_PAGES_HOME_H
