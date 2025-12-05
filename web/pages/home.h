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
extern String modem_type;

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
      // Prominent callsign header
      html += "<div class='callsign-header'>";
      if (activity.srcCallsign.length() > 0) {
        html += "<a href='" + String(QRZ_LOOKUP_URL) + activity.srcCallsign + "' target='_blank' rel='noopener noreferrer'>" + activity.srcCallsign + "</a>";
      } else {
        html += "Unknown";
      }
      html += "</div>";

      html += "<div class='metric'>";
      html += "<span class='metric-label'>DMR ID:</span>";
      html += "<span class='metric-value'>" + String(activity.srcId) + "</span>";
      html += "</div>";

      if (activity.srcName.length() > 0) {
        html += "<div class='metric'>";
        html += "<span class='metric-label'>Name:</span>";
        html += "<span class='metric-value'>" + activity.srcName + "</span>";
        html += "</div>";
      }

      if (activity.srcCity.length() > 0) {
        html += "<div class='metric'>";
        html += "<span class='metric-label'>City:</span>";
        html += "<span class='metric-value'>" + activity.srcCity + "</span>";
        html += "</div>";
      }

      if (activity.srcCountry.length() > 0) {
        html += "<div class='metric'>";
        html += "<span class='metric-label'>Country:</span>";
        html += "<span class='metric-value'>" + activity.srcCountry + "</span>";
        html += "</div>";
      }

      html += "<div class='metric'>";
      html += "<span class='metric-label'>Destination:</span>";
      html += "<span class='metric-value'>";
      if (activity.isGroup) html += "TG ";
      html += String(activity.dstId) + "</span>";
      html += "</div>";

      html += "<div class='metric'>";
      html += "<span class='metric-label'>Type:</span>";
      html += "<span class='metric-value'>" + activity.frameType + "</span>";
      html += "</div>";

      // Calculate transmission duration from start time
      unsigned long duration = (millis() - activity.startTime) / 1000;
      html += "<div class='metric'>";
      html += "<span class='metric-label'>Duration:</span>";
      html += "<span class='metric-value'>" + String(duration) + "s</span>";
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

// Helper function to generate single slot DMR activity HTML
String getDMRSlotHTML(int slotIndex) {
  DMRActivity &activity = dmrActivity[slotIndex];
  String html = "";
  
  String cardClass = activity.active ? "activity-card" : "activity-idle";
  html += "<div class='" + cardClass + "'>";
  html += "<div class='activity-details'>";
  
  if (activity.active) {
    // Prominent callsign header
    html += "<div class='callsign-header'>";
    if (activity.srcCallsign.length() > 0) {
      html += "<a href='" + String(QRZ_LOOKUP_URL) + activity.srcCallsign + "' target='_blank' rel='noopener noreferrer'>" + activity.srcCallsign + "</a>";
    } else {
      html += "Unknown";
    }
    html += "</div>";

    html += "<div class='metric'>";
    html += "<span class='metric-label'>DMR ID:</span>";
    html += "<span class='metric-value'>" + String(activity.srcId) + "</span>";
    html += "</div>";

    if (activity.srcName.length() > 0) {
      html += "<div class='metric'>";
      html += "<span class='metric-label'>Name:</span>";
      html += "<span class='metric-value'>" + activity.srcName + "</span>";
      html += "</div>";
    }

    if (activity.srcCity.length() > 0) {
      html += "<div class='metric'>";
      html += "<span class='metric-label'>City:</span>";
      html += "<span class='metric-value'>" + activity.srcCity + "</span>";
      html += "</div>";
    }

    if (activity.srcCountry.length() > 0) {
      html += "<div class='metric'>";
      html += "<span class='metric-label'>Country:</span>";
      html += "<span class='metric-value'>" + activity.srcCountry + "</span>";
      html += "</div>";
    }

    html += "<div class='metric'>";
    html += "<span class='metric-label'>Destination:</span>";
    html += "<span class='metric-value'>";
    if (activity.isGroup) html += "TG ";
    html += String(activity.dstId) + "</span>";
    html += "</div>";

    html += "<div class='metric'>";
    html += "<span class='metric-label'>Type:</span>";
    html += "<span class='metric-value'>" + activity.frameType + "</span>";
    html += "</div>";

    // Calculate transmission duration from start time
    unsigned long duration = (millis() - activity.startTime) / 1000;
    html += "<div class='metric'>";
    html += "<span class='metric-label'>Duration:</span>";
    html += "<span class='metric-value'>" + String(duration) + "s</span>";
    html += "</div>";
  } else {
    html += "<div class='no-activity'>No Active Transmission</div>";
  }
  
  html += "</div>";
  html += "</div>";
  
  return html;
}

void handleDMRActivity() {
  server.send(200, "text/html", getDMRActivityHTML());
}

void handleDMRSlot1() {
  server.send(200, "text/html", getDMRSlotHTML(0));
}

void handleDMRSlot2() {
  server.send(200, "text/html", getDMRSlotHTML(1));
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
        html += "<div class='history-row' data-duration='" + String(dmrHistory[index].duration) + "'>";
        
        // Time
        html += "<div class='col-time'>" + dmrHistory[index].timestamp + "</div>";
        
        // Station info
        html += "<div class='col-station'>";
        if (dmrHistory[index].srcCallsign.length() > 0) {
          html += "<div class='callsign'><a href='" + String(QRZ_LOOKUP_URL) + dmrHistory[index].srcCallsign + "' target='_blank' rel='noopener noreferrer'>" + dmrHistory[index].srcCallsign + "</a></div>";
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
  // Activity cards grid for side-by-side layout
  html += ".activity-cards-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 15px; margin-bottom: 20px; }";
  html += ".activity-single-card { margin: 0; }";
  html += ".activity-card { border-left: 4px solid #4CAF50; animation: pulse 2s infinite; background: var(--card-bg); }";
  html += ".activity-idle { border-left: 4px solid var(--border-color); opacity: 0.6; background: var(--card-bg); }";
  html += "@keyframes pulse { 0%, 100% { border-color: #4CAF50; } 50% { border-color: #81C784; } }";
  html += ".activity-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 15px; }";
  html += ".slot-header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 10px; border-radius: 5px 5px 0 0; font-weight: bold; }";
  html += ".activity-details { padding: 15px; background: var(--container-bg); border: 1px solid var(--border-color); border-radius: 5px; }";
  html += ".callsign-header { font-size: 1.5em; font-weight: bold; margin-bottom: 15px; padding-bottom: 10px; border-bottom: 2px solid var(--border-color); }";
  html += ".callsign-header a { color: var(--link-color); text-decoration: none; transition: color 0.2s; }";
  html += ".callsign-header a:hover { color: #64b5f6; text-decoration: underline; }";
  html += ".activity-details .metric-value a { color: var(--link-color); text-decoration: none; font-weight: bold; transition: color 0.2s; }";
  html += ".activity-details .metric-value a:hover { color: #64b5f6; text-decoration: underline; }";
  html += ".no-activity { text-align: center; padding: 30px; color: var(--text-color); font-style: italic; opacity: 0.6; }";
  // History card styles (with dark/light mode support)
  html += ".history-card { margin-top: 20px; }";
  html += ".history-title-bar { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; flex-wrap: wrap; }";
  html += ".history-title-bar h3 { margin: 0; color: var(--text-color); }";
  html += ".filter-controls { display: flex; align-items: center; gap: 8px; }";
  html += ".filter-controls label { font-size: 0.9em; color: var(--text-color); white-space: nowrap; opacity: 0.8; }";
  html += ".filter-controls select { padding: 4px 8px; border: 1px solid var(--border-color); border-radius: 4px; font-size: 0.9em; background: var(--container-bg); color: var(--text-color); }";
  html += ".history-container { overflow-x: auto; }";
  html += ".history-header { display: grid; grid-template-columns: 80px 1fr 120px 80px 50px; gap: 10px; padding: 10px; background: var(--card-bg); border: 1px solid var(--border-color); font-weight: bold; border-radius: 5px; margin-bottom: 5px; color: var(--text-color); }";
  html += ".history-row { display: grid; grid-template-columns: 80px 1fr 120px 80px 50px; gap: 10px; padding: 8px 10px; border-bottom: 1px solid var(--border-color); transition: background-color 0.2s; }";
  html += ".history-row:nth-child(even) { background: var(--card-bg); }";
  html += ".history-row:hover { background: var(--info-bg); }";
  html += ".history-row.filtered-out { display: none; }";
  html += ".col-time { font-family: monospace; font-size: 0.9em; color: var(--text-color); }";
  html += ".col-station .callsign { font-weight: bold; color: var(--link-color); }";
  html += ".col-station .callsign a { color: var(--link-color); text-decoration: none; transition: color 0.2s; }";
  html += ".col-station .callsign a:hover { color: #64b5f6; text-decoration: underline; }";
  html += ".col-station .name { font-size: 0.85em; color: var(--text-color); opacity: 0.8; }";
  html += ".col-station .location { font-size: 0.8em; color: var(--text-color); opacity: 0.6; }";
  html += ".col-destination { font-family: monospace; font-weight: bold; color: var(--text-color); }";
  html += ".col-duration { text-align: center; font-family: monospace; color: var(--text-color); }";
  html += ".col-slot { text-align: center; font-weight: bold; color: var(--text-color); }";
  html += ".no-history { text-align: center; padding: 40px; color: var(--text-color); font-style: italic; opacity: 0.6; }";
  html += "@media (max-width: 768px) { .history-title-bar { flex-direction: column; align-items: flex-start; gap: 10px; } .filter-controls { align-self: flex-end; } .history-header, .history-row { grid-template-columns: 60px 1fr 80px 50px; } .col-slot { display: none; } }";
  html += "</style>";
  html += "<script>";
  html += "function refreshActivity() {";
  html += "  fetch('/dmr-slot1').then(r => r.text()).then(data => {";
  html += "    document.getElementById('dmr-activity-slot1').innerHTML = data;";
  html += "  });";
  html += "  fetch('/dmr-slot2').then(r => r.text()).then(data => {";
  html += "    document.getElementById('dmr-activity-slot2').innerHTML = data;";
  html += "  });";
  html += "}";
  html += "function refreshHistory() {";
  html += "  fetch('/dmr-history').then(r => r.text()).then(data => {";
  html += "    document.getElementById('dmr-history-content').innerHTML = data;";
  html += "    filterHistory();";
  html += "  });";
  html += "}";
  html += "function filterHistory() {";
  html += "  const filterValue = parseFloat(document.getElementById('duration-filter').value);";
  html += "  const rows = document.querySelectorAll('.history-row');";
  html += "  let visibleCount = 0;";
  html += "  rows.forEach(row => {";
  html += "    const durationCell = row.querySelector('.col-duration');";
  html += "    if (durationCell) {";
  html += "      const durationText = durationCell.textContent.trim();";
  html += "      const duration = parseFloat(durationText.replace('s', ''));";
  html += "      if (filterValue > 0 && duration < filterValue) {";
  html += "        row.classList.add('filtered-out');";
  html += "      } else {";
  html += "        row.classList.remove('filtered-out');";
  html += "        visibleCount++;";
  html += "      }";
  html += "    }";
  html += "  });";
  html += "  const container = document.querySelector('.history-container');";
  html += "  let noResultsMsg = container.querySelector('.no-results-filtered');";
  html += "  if (visibleCount === 0 && rows.length > 0) {";
  html += "    if (!noResultsMsg) {";
  html += "      noResultsMsg = document.createElement('div');";
  html += "      noResultsMsg.className = 'no-results-filtered no-history';";
  html += "      noResultsMsg.innerHTML = 'No transmissions match the current filter';";
  html += "      container.appendChild(noResultsMsg);";
  html += "    }";
  html += "  } else if (noResultsMsg) {";
  html += "    noResultsMsg.remove();";
  html += "  }";
  html += "}";
  html += "setInterval(refreshActivity, 1000);";
  html += "setInterval(refreshHistory, 2000);";
  html += "</script>";
  html += "</head><body>";
  html += getNavigation("main");
  html += "<div class='container'>";
  html += "<h1><center>" + dmr_callsign + " - ESP32 MMDVM Hotspot</center></h1>";

  // Live DMR Activity Section - Split into two cards for better mobile responsiveness
  html += "<div class='activity-cards-grid'>";

  // Check if modem supports dual slots (Dual_Hat variants)
  bool isDualSlotModem = (modem_type.indexOf("dual") >= 0 || modem_type.indexOf("Dual") >= 0);

  // Slot 1 Card - Only show for dual-slot modems
  if (isDualSlotModem) {
    html += "<div class='card activity-single-card'>";
    html += "<h3>Live DMR Activity - Slot 1</h3>";
    html += "<div id='dmr-activity-slot1'>";
    html += getDMRSlotHTML(0); // Slot 1 = index 0
    html += "</div>";
    html += "</div>";
  }

  // Slot 2 Card - Always show (all modems use Slot 2)
  html += "<div class='card activity-single-card'>";
  html += "<h3>Live DMR Activity - Slot 2</h3>";
  html += "<div id='dmr-activity-slot2'>";
  html += getDMRSlotHTML(1); // Slot 2 = index 1
  html += "</div>";
  html += "</div>";

  html += "</div>";

  // Recent Activity History Card (full width)
  html += "<div class='card history-card'>";
  html += "<div class='history-title-bar'>";
  html += "<h3>Recent DMR Activity (Last 15 Transmissions)</h3>";
  html += "<div class='filter-controls'>";
  html += "<label for='duration-filter'>Hide transmissions under:</label>";
  html += "<select id='duration-filter' onchange='filterHistory()'>";
  html += "<option value='0'>Show all</option>";
  html += "<option value='0.5'>0.5 seconds</option>";
  html += "<option value='1'>1 second</option>";
  html += "<option value='2'>2 seconds</option>";
  html += "<option value='3'>3 seconds</option>";
  html += "<option value='5'>5 seconds</option>";
  html += "</select>";
  html += "</div>";
  html += "</div>";
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
