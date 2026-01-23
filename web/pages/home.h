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
#if USE_ETHERNET
extern bool eth_connected;
#endif
extern bool mode_dmr_enabled;
extern bool mode_dstar_enabled;
extern bool mode_ysf_enabled;
extern bool mode_p25_enabled;
extern bool mode_nxdn_enabled;
extern bool mode_pocsag_enabled;
extern bool sdCardAvailable;
extern bool mqtt_enabled;
extern bool mqttConnected;

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

// RF Activity History structure (matches esp32_mmdvm_hotspot.ino)
struct RFActivityHistory {
  uint32_t srcId;
  String srcCallsign;
  uint32_t dstId;
  bool isGroup;
  uint32_t duration;
  uint8_t slotNo;
  unsigned long timestamp;
};
extern RFActivityHistory rfHistory[15];
extern int rfHistoryIndex;

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
    html += "<span class='metric-label'>DMR ID:</span>";
    html += "<span class='metric-value'>" + String(activity.srcId) + "</span>";
    html += "</div>";

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
  // Return JSON for both slots
  String json = "{\"slots\":[";

  for (int i = 0; i < 2; i++) {
    DMRActivity &activity = dmrActivity[i];
    if (i > 0) json += ",";

    json += "{";
    json += "\"slotNo\":" + String(activity.slotNo) + ",";
    json += "\"active\":" + String(activity.active ? "true" : "false");

    if (activity.active) {
      json += ",\"srcId\":" + String(activity.srcId);
      json += ",\"srcCallsign\":\"" + activity.srcCallsign + "\"";
      json += ",\"srcName\":\"" + activity.srcName + "\"";
      json += ",\"srcCity\":\"" + activity.srcCity + "\"";
      json += ",\"srcCountry\":\"" + activity.srcCountry + "\"";
      json += ",\"dstId\":" + String(activity.dstId);
      json += ",\"isGroup\":" + String(activity.isGroup ? "true" : "false");
      json += ",\"frameType\":\"" + activity.frameType + "\"";
      json += ",\"duration\":" + String((millis() - activity.startTime) / 1000);
    }

    json += "}";
  }

  json += "]}";
  server.send(200, "application/json", json);
}

void handleDMRSlot1() {
  DMRActivity &activity = dmrActivity[0];

  String json = "{";
  json += "\"slotNo\":" + String(activity.slotNo) + ",";
  json += "\"active\":" + String(activity.active ? "true" : "false");

  if (activity.active) {
    json += ",\"srcId\":" + String(activity.srcId);
    json += ",\"srcCallsign\":\"" + activity.srcCallsign + "\"";
    json += ",\"srcName\":\"" + activity.srcName + "\"";
    json += ",\"srcCity\":\"" + activity.srcCity + "\"";
    json += ",\"srcCountry\":\"" + activity.srcCountry + "\"";
    json += ",\"dstId\":" + String(activity.dstId);
    json += ",\"isGroup\":" + String(activity.isGroup ? "true" : "false");
    json += ",\"frameType\":\"" + activity.frameType + "\"";
    json += ",\"duration\":" + String((millis() - activity.startTime) / 1000);
  }

  json += "}";
  server.send(200, "application/json", json);
}

void handleDMRSlot2() {
  DMRActivity &activity = dmrActivity[1];

  String json = "{";
  json += "\"slotNo\":" + String(activity.slotNo) + ",";
  json += "\"active\":" + String(activity.active ? "true" : "false");

  if (activity.active) {
    json += ",\"srcId\":" + String(activity.srcId);
    json += ",\"srcCallsign\":\"" + activity.srcCallsign + "\"";
    json += ",\"srcName\":\"" + activity.srcName + "\"";
    json += ",\"srcCity\":\"" + activity.srcCity + "\"";
    json += ",\"srcCountry\":\"" + activity.srcCountry + "\"";
    json += ",\"dstId\":" + String(activity.dstId);
    json += ",\"isGroup\":" + String(activity.isGroup ? "true" : "false");
    json += ",\"frameType\":\"" + activity.frameType + "\"";
    json += ",\"duration\":" + String((millis() - activity.startTime) / 1000);
  }

  json += "}";
  server.send(200, "application/json", json);
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
  String json = "{\"history\":[";

  bool firstEntry = true;
  // Show entries in reverse chronological order (newest first)
  for (int i = 0; i < 15; i++) {
    int index = (dmrHistoryIndex - 1 - i + 15) % 15;
    if (dmrHistory[index].srcId > 0) {
      if (!firstEntry) json += ",";
      firstEntry = false;

      json += "{";
      json += "\"timestamp\":\"" + dmrHistory[index].timestamp + "\",";
      json += "\"srcId\":" + String(dmrHistory[index].srcId) + ",";
      json += "\"srcCallsign\":\"" + dmrHistory[index].srcCallsign + "\",";
      json += "\"srcName\":\"" + dmrHistory[index].srcName + "\",";
      json += "\"srcLocation\":\"" + dmrHistory[index].srcLocation + "\",";
      json += "\"dstId\":" + String(dmrHistory[index].dstId) + ",";
      json += "\"isGroup\":" + String(dmrHistory[index].isGroup ? "true" : "false") + ",";
      json += "\"duration\":" + String(dmrHistory[index].duration) + ",";
      json += "\"ber\":" + String(dmrHistory[index].ber) + ",";
      json += "\"rssi\":" + String(dmrHistory[index].rssi) + ",";
      json += "\"slotNo\":" + String(dmrHistory[index].slotNo);
      json += "}";
    }
  }

  json += "]}";
  server.send(200, "application/json", json);
}

// Helper function to generate Local RF Activity HTML
String getRFHistoryHTML() {
  String html = "<div class='history-container'>";

  // Count actual history entries
  int entryCount = 0;
  for (int i = 0; i < 15; i++) {
    if (rfHistory[i].srcId > 0) entryCount++;
  }

  if (entryCount == 0) {
    html += "<div class='no-history'>No local RF transmissions yet</div>";
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
      int index = (rfHistoryIndex - 1 - i + 15) % 15;
      if (rfHistory[index].srcId > 0) {
        // Calculate time ago
        unsigned long secondsAgo = (millis() - rfHistory[index].timestamp) / 1000;
        String timeAgo;
        if (secondsAgo < 60) {
          timeAgo = String(secondsAgo) + "s ago";
        } else if (secondsAgo < 3600) {
          timeAgo = String(secondsAgo / 60) + "m ago";
        } else {
          timeAgo = String(secondsAgo / 3600) + "h ago";
        }

        html += "<div class='history-row rf-history-row' data-duration='" + String(rfHistory[index].duration) + "'>";

        // Time
        html += "<div class='col-time'>" + timeAgo + "</div>";

        // Station info (local station)
        html += "<div class='col-station'>";
        if (rfHistory[index].srcCallsign.length() > 0) {
          html += "<div class='callsign'>";
          html += "<a href='https://www.qrz.com/db/" + rfHistory[index].srcCallsign + "' target='_blank'>" + rfHistory[index].srcCallsign + "</a>";
          html += "</div>";
          html += "<div class='name'>" + String(rfHistory[index].srcId) + "</div>";
        } else {
          html += "<div class='callsign'>" + String(rfHistory[index].srcId) + "</div>";
        }
        html += "</div>";

        // Destination
        html += "<div class='col-destination'>";
        if (rfHistory[index].isGroup) html += "TG ";
        html += String(rfHistory[index].dstId) + "</div>";

        // Duration
        html += "<div class='col-duration'>" + String(rfHistory[index].duration) + "s</div>";

        // Slot
        html += "<div class='col-slot'>" + String(rfHistory[index].slotNo) + "</div>";

        html += "</div>";
      }
    }
  }

  html += "</div>";
  return html;
}

void handleRFHistory() {
  String json = "{\"history\":[";

  bool firstEntry = true;
  // Show entries in reverse chronological order (newest first)
  for (int i = 0; i < 15; i++) {
    int index = (rfHistoryIndex - 1 - i + 15) % 15;
    if (rfHistory[index].srcId > 0) {
      if (!firstEntry) json += ",";
      firstEntry = false;

      // Calculate time ago
      unsigned long secondsAgo = (millis() - rfHistory[index].timestamp) / 1000;

      json += "{";
      json += "\"secondsAgo\":" + String(secondsAgo) + ",";
      json += "\"srcId\":" + String(rfHistory[index].srcId) + ",";
      json += "\"srcCallsign\":\"" + rfHistory[index].srcCallsign + "\",";
      json += "\"dstId\":" + String(rfHistory[index].dstId) + ",";
      json += "\"isGroup\":" + String(rfHistory[index].isGroup ? "true" : "false") + ",";
      json += "\"duration\":" + String(rfHistory[index].duration) + ",";
      json += "\"slotNo\":" + String(rfHistory[index].slotNo);
      json += "}";
    }
  }

  json += "]}";
  server.send(200, "application/json", json);
}

// Helper function to generate System Status HTML
String getSystemStatusHTML() {
  String html = "";

  // Network Connectivity Section
  html += "<div class='status-section'>";
  html += "<div class='status-section-title'>Network Connectivity</div>";
  html += "<div class='status-badges'>";

  // WiFi Status
  if (wifiConnected) {
    html += "<span class='status-badge badge-active'><span class='status-dot dot-green'></span> WiFi Connected</span>";
  } else if (apMode) {
    html += "<span class='status-badge badge-warning'><span class='status-dot dot-yellow'></span> WiFi (AP Mode)</span>";
  } else {
    html += "<span class='status-badge badge-inactive'><span class='status-dot dot-red'></span> WiFi Disconnected</span>";
  }

  // Ethernet Status (only on supported hardware)
#if USE_ETHERNET
  if (eth_connected) {
    html += "<span class='status-badge badge-active'><span class='status-dot dot-green'></span> Ethernet</span>";
  } else {
    html += "<span class='status-badge badge-inactive'><span class='status-dot dot-red'></span> Ethernet</span>";
  }
#endif

  // MQTT Status - Always show, ON/OFF based on mqtt_enabled, color based on mqttConnected
  if (mqtt_enabled) {
    if (mqttConnected) {
      html += "<span class='status-badge badge-active'><span class='status-dot dot-green'></span> MQTT ON</span>";
    } else {
      html += "<span class='status-badge badge-inactive'><span class='status-dot dot-red'></span> MQTT ON</span>";
    }
  } else {
    html += "<span class='status-badge badge-inactive'><span class='status-dot dot-red'></span> MQTT OFF</span>";
  }

  html += "</div></div>";

  // Hardware Status Section
  html += "<div class='status-section'>";
  html += "<div class='status-section-title'>Hardware Status</div>";
  html += "<div class='status-badges'>";

  // MMDVM Hardware
  if (mmdvmReady) {
    html += "<span class='status-badge badge-active'><span class='status-dot dot-green'></span> MMDVM Ready</span>";
  } else {
    html += "<span class='status-badge badge-inactive'><span class='status-dot dot-red'></span> MMDVM Not Ready</span>";
  }


  // SD Card Status
#if USE_SD_CARD
  if (sdCardAvailable) {
    html += "<span class='status-badge badge-active'><span class='status-dot dot-green'></span> SD Card</span>";
  } else {
    html += "<span class='status-badge badge-inactive'><span class='status-dot dot-red'></span> SD Card</span>";
  }
#endif

  html += "</div></div>";

  // Network Status Section
  html += "<div class='status-section'>";
  html += "<div class='status-section-title'>Network Status</div>";
  html += "<div class='status-badges'>";

  // // MMDVM Hardware
  // if (mmdvmReady) {
  //   html += "<span class='status-badge badge-active'><span class='status-dot dot-green'></span> MMDVM Ready</span>";
  // } else {
  //   html += "<span class='status-badge badge-inactive'><span class='status-dot dot-red'></span> MMDVM Not Ready</span>";
  // }

  // DMR Network
  if (dmrLoggedIn) {
    html += "<span class='status-badge badge-active'><span class='status-dot dot-green'></span> DMR Network</span>";
  } else {
    html += "<span class='status-badge badge-inactive'><span class='status-dot dot-red'></span> DMR Network</span>";
  }

  html += "</div></div>";

  // Modes Section
  html += "<div class='status-section'>";
  html += "<div class='status-section-title'>Digital Modes</div>";
  html += "<div class='status-badges'>";

  // DMR Mode - with toggle switch
  if (mode_dmr_enabled) {
    html += "<span class='mode-badge mode-on'>";
    html += "<span class='toggle-switch toggle-switch-on'><span class='toggle-knob toggle-knob-on'></span></span>";
    html += "DMR ON</span>";
  } else {
    html += "<span class='mode-badge mode-off'>";
    html += "<span class='toggle-switch toggle-switch-off'><span class='toggle-knob toggle-knob-off'></span></span>";
    html += "DMR OFF</span>";
  }

  // D-Star Mode (not implemented) - grayed out
  if (mode_dstar_enabled) {
    html += "<span class='mode-badge mode-disabled'>";
    html += "<span class='toggle-switch toggle-switch-disabled'><span class='toggle-knob toggle-knob-on'></span></span>";
    html += "D-Star (N/A)</span>";
  } else {
    html += "<span class='mode-badge mode-disabled'>";
    html += "<span class='toggle-switch toggle-switch-disabled'><span class='toggle-knob toggle-knob-off'></span></span>";
    html += "D-Star (N/A)</span>";
  }

  // YSF Mode (not implemented) - grayed out
  if (mode_ysf_enabled) {
    html += "<span class='mode-badge mode-disabled'>";
    html += "<span class='toggle-switch toggle-switch-disabled'><span class='toggle-knob toggle-knob-on'></span></span>";
    html += "YSF (N/A)</span>";
  } else {
    html += "<span class='mode-badge mode-disabled'>";
    html += "<span class='toggle-switch toggle-switch-disabled'><span class='toggle-knob toggle-knob-off'></span></span>";
    html += "YSF (N/A)</span>";
  }

  // P25 Mode (not implemented) - grayed out
  if (mode_p25_enabled) {
    html += "<span class='mode-badge mode-disabled'>";
    html += "<span class='toggle-switch toggle-switch-disabled'><span class='toggle-knob toggle-knob-on'></span></span>";
    html += "P25 (N/A)</span>";
  } else {
    html += "<span class='mode-badge mode-disabled'>";
    html += "<span class='toggle-switch toggle-switch-disabled'><span class='toggle-knob toggle-knob-off'></span></span>";
    html += "P25 (N/A)</span>";
  }

  // NXDN Mode (not implemented) - grayed out
  if (mode_nxdn_enabled) {
    html += "<span class='mode-badge mode-disabled'>";
    html += "<span class='toggle-switch toggle-switch-disabled'><span class='toggle-knob toggle-knob-on'></span></span>";
    html += "NXDN (N/A)</span>";
  } else {
    html += "<span class='mode-badge mode-disabled'>";
    html += "<span class='toggle-switch toggle-switch-disabled'><span class='toggle-knob toggle-knob-off'></span></span>";
    html += "NXDN (N/A)</span>";
  }

  // POCSAG Mode (not implemented) - grayed out
  if (mode_pocsag_enabled) {
    html += "<span class='mode-badge mode-disabled'>";
    html += "<span class='toggle-switch toggle-switch-disabled'><span class='toggle-knob toggle-knob-on'></span></span>";
    html += "POCSAG (N/A)</span>";
  } else {
    html += "<span class='mode-badge mode-disabled'>";
    html += "<span class='toggle-switch toggle-switch-disabled'><span class='toggle-knob toggle-knob-off'></span></span>";
    html += "POCSAG (N/A)</span>";
  }

  html += "</div></div>";

  return html;
}

void handleSystemStatus() {
  String json = "{";

  // Network connectivity
  json += "\"network\":{";
  json += "\"wifiConnected\":" + String(wifiConnected ? "true" : "false") + ",";
  json += "\"apMode\":" + String(apMode ? "true" : "false");
#if USE_ETHERNET
  json += ",\"ethConnected\":" + String(eth_connected ? "true" : "false");
#endif
  json += ",\"mqttEnabled\":" + String(mqtt_enabled ? "true" : "false");
  json += ",\"mqttConnected\":" + String(mqttConnected ? "true" : "false");
  json += "},";

  // SD Card status
  json += "\"sdCard\":{";
#if USE_SD_CARD
  json += "\"available\":" + String(sdCardAvailable ? "true" : "false");
#else
  json += "\"available\":false";
#endif
  json += "},";

  // System status
  json += "\"system\":{";
  json += "\"mmdvmReady\":" + String(mmdvmReady ? "true" : "false") + ",";
  json += "\"dmrLoggedIn\":" + String(dmrLoggedIn ? "true" : "false");
  json += "},";

  // Digital modes
  json += "\"modes\":{";
  json += "\"dmr\":" + String(mode_dmr_enabled ? "true" : "false") + ",";
  json += "\"dstar\":" + String(mode_dstar_enabled ? "true" : "false") + ",";
  json += "\"ysf\":" + String(mode_ysf_enabled ? "true" : "false") + ",";
  json += "\"p25\":" + String(mode_p25_enabled ? "true" : "false") + ",";
  json += "\"nxdn\":" + String(mode_nxdn_enabled ? "true" : "false") + ",";
  json += "\"pocsag\":" + String(mode_pocsag_enabled ? "true" : "false");
  json += "}";

  json += "}";
  server.send(200, "application/json", json);
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
  html += ".filter-controls { display: flex; align-items: center; gap: 8px; flex-wrap: wrap; }";
  html += ".filter-controls label { font-size: 0.9em; color: var(--text-color); white-space: nowrap; opacity: 0.8; }";
  html += ".filter-controls select { padding: 4px 8px; border: 1px solid var(--border-color); border-radius: 4px; font-size: 0.9em; background: var(--container-bg); color: var(--text-color); min-width: 120px; max-width: 100%; }";
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
  html += "@media (max-width: 768px) { .history-title-bar { flex-direction: column; align-items: flex-start; gap: 10px; } .filter-controls { align-self: stretch; width: 100%; } .filter-controls label { flex-shrink: 0; } .filter-controls select { flex: 1; min-width: 0; } .history-header, .history-row { grid-template-columns: 60px 1fr 80px 50px; } .col-slot { display: none; } }";
  html += "</style>";
  html += "<script>";
  // Configuration constants
#if USE_SD_CARD
  html += "const USE_SD_CARD = true;";
#else
  html += "const USE_SD_CARD = false;";
#endif
  // Render functions for JSON data
  html += "function renderDMRSlot(data) {";
  html += "  let html = '';";
  html += "  let cardClass = data.active ? 'activity-card' : 'activity-idle';";
  html += "  html += '<div class=\"' + cardClass + '\">';";
  html += "  html += '<div class=\"activity-details\">';";
  html += "  if (data.active) {";
  html += "    html += '<div class=\"callsign-header\">';";
  html += "    if (data.srcCallsign && data.srcCallsign.length > 0) {";
  html += "      html += '<a href=\"https://www.qrz.com/db/' + data.srcCallsign + '\" target=\"_blank\" rel=\"noopener noreferrer\">' + data.srcCallsign + '</a>';";
  html += "    } else {";
  html += "      html += 'Unknown';";
  html += "    }";
  html += "    html += '</div>';";
  html += "    if (data.srcName && data.srcName.length > 0) html += '<div class=\"metric\"><span class=\"metric-label\">Name:</span><span class=\"metric-value\">' + data.srcName + '</span></div>';";
  html += "    if (data.srcCity && data.srcCity.length > 0) html += '<div class=\"metric\"><span class=\"metric-label\">City:</span><span class=\"metric-value\">' + data.srcCity + '</span></div>';";
  html += "    if (data.srcCountry && data.srcCountry.length > 0) html += '<div class=\"metric\"><span class=\"metric-label\">Country:</span><span class=\"metric-value\">' + data.srcCountry + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">DMR ID:</span><span class=\"metric-value\">' + data.srcId + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Destination:</span><span class=\"metric-value\">' + (data.isGroup ? 'TG ' : '') + data.dstId + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Type:</span><span class=\"metric-value\">' + data.frameType + '</span></div>';";
  html += "    html += '<div class=\"metric\"><span class=\"metric-label\">Duration:</span><span class=\"metric-value\">' + data.duration + 's</span></div>';";
  html += "  } else {";
  html += "    html += '<div class=\"no-activity\">No Active Transmission</div>';";
  html += "  }";
  html += "  html += '</div></div>';";
  html += "  return html;";
  html += "}";
  html += "function renderDMRHistory(data) {";
  html += "  let html = '<div class=\"history-container\">';";
  html += "  if (data.history.length === 0) {";
  html += "    html += '<div class=\"no-history\">No recent transmissions</div>';";
  html += "  } else {";
  html += "    html += '<div class=\"history-header\">';";
  html += "    html += '<div class=\"col-time\">Time</div>';";
  html += "    html += '<div class=\"col-station\">Station</div>';";
  html += "    html += '<div class=\"col-destination\">Destination</div>';";
  html += "    html += '<div class=\"col-duration\">Duration</div>';";
  html += "    html += '<div class=\"col-slot\">Slot</div>';";
  html += "    html += '</div>';";
  html += "    data.history.forEach(entry => {";
  html += "      html += '<div class=\"history-row\" data-duration=\"' + entry.duration + '\">';";
  html += "      html += '<div class=\"col-time\">' + entry.timestamp + '</div>';";
  html += "      html += '<div class=\"col-station\">';";
  html += "      if (entry.srcCallsign && entry.srcCallsign.length > 0) {";
  html += "        html += '<div class=\"callsign\"><a href=\"https://www.qrz.com/db/' + entry.srcCallsign + '\" target=\"_blank\" rel=\"noopener noreferrer\">' + entry.srcCallsign + '</a></div>';";
  html += "        if (entry.srcName && entry.srcName.length > 0) html += '<div class=\"name\">' + entry.srcName + '</div>';";
  html += "        if (entry.srcLocation && entry.srcLocation.length > 0) html += '<div class=\"location\">' + entry.srcLocation + '</div>';";
  html += "      } else {";
  html += "        html += '<div class=\"callsign\">' + entry.srcId + '</div>';";
  html += "      }";
  html += "      html += '</div>';";
  html += "      html += '<div class=\"col-destination\">' + (entry.isGroup ? 'TG ' : '') + entry.dstId + '</div>';";
  html += "      html += '<div class=\"col-duration\">' + entry.duration + 's</div>';";
  html += "      html += '<div class=\"col-slot\">' + entry.slotNo + '</div>';";
  html += "      html += '</div>';";
  html += "    });";
  html += "  }";
  html += "  html += '</div>';";
  html += "  return html;";
  html += "}";
  html += "function renderRFHistory(data) {";
  html += "  let html = '<div class=\"history-container\">';";
  html += "  if (data.history.length === 0) {";
  html += "    html += '<div class=\"no-history\">No local RF transmissions yet</div>';";
  html += "  } else {";
  html += "    html += '<div class=\"history-header\">';";
  html += "    html += '<div class=\"col-time\">Time</div>';";
  html += "    html += '<div class=\"col-station\">Station</div>';";
  html += "    html += '<div class=\"col-destination\">Destination</div>';";
  html += "    html += '<div class=\"col-duration\">Duration</div>';";
  html += "    html += '<div class=\"col-slot\">Slot</div>';";
  html += "    html += '</div>';";
  html += "    data.history.forEach(entry => {";
  html += "      let timeAgo = entry.secondsAgo < 60 ? entry.secondsAgo + 's ago' : entry.secondsAgo < 3600 ? Math.floor(entry.secondsAgo / 60) + 'm ago' : Math.floor(entry.secondsAgo / 3600) + 'h ago';";
  html += "      html += '<div class=\"history-row rf-history-row\" data-duration=\"' + entry.duration + '\">';";
  html += "      html += '<div class=\"col-time\">' + timeAgo + '</div>';";
  html += "      html += '<div class=\"col-station\">';";
  html += "      if (entry.srcCallsign && entry.srcCallsign.length > 0) {";
  html += "        html += '<div class=\"callsign\"><a href=\"https://www.qrz.com/db/' + entry.srcCallsign + '\" target=\"_blank\">' + entry.srcCallsign + '</a></div>';";
  html += "        html += '<div class=\"name\">' + entry.srcId + '</div>';";
  html += "      } else {";
  html += "        html += '<div class=\"callsign\">' + entry.srcId + '</div>';";
  html += "      }";
  html += "      html += '</div>';";
  html += "      html += '<div class=\"col-destination\">' + (entry.isGroup ? 'TG ' : '') + entry.dstId + '</div>';";
  html += "      html += '<div class=\"col-duration\">' + entry.duration + 's</div>';";
  html += "      html += '<div class=\"col-slot\">' + entry.slotNo + '</div>';";
  html += "      html += '</div>';";
  html += "    });";
  html += "  }";
  html += "  html += '</div>';";
  html += "  return html;";
  html += "}";
  html += "function renderSystemStatus(data) {";
  html += "  let html = '';";
  html += "  html += '<div class=\"status-section\"><div class=\"status-section-title\">Network Connectivity</div><div class=\"status-badges\">';";
  html += "  if (data.network.wifiConnected) {";
  html += "    html += '<span class=\"status-badge badge-active\"><span class=\"status-dot dot-green\"></span> WiFi Connected</span>';";
  html += "  } else if (data.network.apMode) {";
  html += "    html += '<span class=\"status-badge badge-warning\"><span class=\"status-dot dot-yellow\"></span> WiFi (AP Mode)</span>';";
  html += "  } else {";
  html += "    html += '<span class=\"status-badge badge-inactive\"><span class=\"status-dot dot-red\"></span> WiFi Disconnected</span>';";
  html += "  }";
  html += "  if (data.network.ethConnected !== undefined) {";
  html += "    if (data.network.ethConnected) {";
  html += "      html += '<span class=\"status-badge badge-active\"><span class=\"status-dot dot-green\"></span> Ethernet</span>';";
  html += "    } else {";
  html += "      html += '<span class=\"status-badge badge-inactive\"><span class=\"status-dot dot-red\"></span> Ethernet</span>';";
  html += "    }";
  html += "  }";
  html += "  if (data.network.mqttEnabled) {";
  html += "    if (data.network.mqttConnected) {";
  html += "      html += '<span class=\"status-badge badge-active\"><span class=\"status-dot dot-green\"></span> MQTT ON</span>';";
  html += "    } else {";
  html += "      html += '<span class=\"status-badge badge-inactive\"><span class=\"status-dot dot-red\"></span> MQTT ON</span>';";
  html += "    }";
  html += "  } else {";
  html += "    html += '<span class=\"status-badge badge-inactive\"><span class=\"status-dot dot-red\"></span> MQTT OFF</span>';";
  html += "  }";
  html += "  html += '</div></div>';";
  html += "  html += '<div class=\"status-section\"><div class=\"status-section-title\">Hardware Status</div><div class=\"status-badges\">';";
  html += "  if (data.system.mmdvmReady) {";
  html += "    html += '<span class=\"status-badge badge-active\"><span class=\"status-dot dot-green\"></span> MMDVM Ready</span>';";
  html += "  } else {";
  html += "    html += '<span class=\"status-badge badge-inactive\"><span class=\"status-dot dot-red\"></span> MMDVM Not Ready</span>';";
  html += "  }";
  html += "  if (USE_SD_CARD) {";
  html += "    if (data.sdCard.available) {";
  html += "      html += '<span class=\"status-badge badge-active\"><span class=\"status-dot dot-green\"></span> SD Card</span>';";
  html += "    } else {";
  html += "      html += '<span class=\"status-badge badge-inactive\"><span class=\"status-dot dot-red\"></span> SD Card</span>';";
  html += "    }";
  html += "  }";
  html += "  html += '</div></div>';";
  html += "  html += '<div class=\"status-section\"><div class=\"status-section-title\">Network Status</div><div class=\"status-badges\">';";
  // html += "  if (data.system.mmdvmReady) {";
  // html += "    html += '<span class=\"status-badge badge-active\"><span class=\"status-dot dot-green\"></span> MMDVM Ready</span>';";
  // html += "  } else {";
  // html += "    html += '<span class=\"status-badge badge-inactive\"><span class=\"status-dot dot-red\"></span> MMDVM Not Ready</span>';";
  // html += "  }";
  html += "  if (data.system.dmrLoggedIn) {";
  html += "    html += '<span class=\"status-badge badge-active\"><span class=\"status-dot dot-green\"></span> DMR Network</span>';";
  html += "  } else {";
  html += "    html += '<span class=\"status-badge badge-inactive\"><span class=\"status-dot dot-red\"></span> DMR Network</span>';";
  html += "  }";
  html += "  html += '</div></div>';";
  html += "  html += '<div class=\"status-section\"><div class=\"status-section-title\">Digital Modes</div><div class=\"status-badges\">';";
  html += "  html += data.modes.dmr ? '<span class=\"mode-badge mode-on\"><span class=\"toggle-switch toggle-switch-on\"><span class=\"toggle-knob toggle-knob-on\"></span></span>DMR ON</span>' : '<span class=\"mode-badge mode-off\"><span class=\"toggle-switch toggle-switch-off\"><span class=\"toggle-knob toggle-knob-off\"></span></span>DMR OFF</span>';";
  html += "  html += '<span class=\"mode-badge mode-disabled\"><span class=\"toggle-switch toggle-switch-disabled\"><span class=\"toggle-knob toggle-knob-off\"></span></span>D-Star (N/A)</span>';";
  html += "  html += '<span class=\"mode-badge mode-disabled\"><span class=\"toggle-switch toggle-switch-disabled\"><span class=\"toggle-knob toggle-knob-off\"></span></span>YSF (N/A)</span>';";
  html += "  html += '<span class=\"mode-badge mode-disabled\"><span class=\"toggle-switch toggle-switch-disabled\"><span class=\"toggle-knob toggle-knob-off\"></span></span>P25 (N/A)</span>';";
  html += "  html += '<span class=\"mode-badge mode-disabled\"><span class=\"toggle-switch toggle-switch-disabled\"><span class=\"toggle-knob toggle-knob-off\"></span></span>NXDN (N/A)</span>';";
  html += "  html += '<span class=\"mode-badge mode-disabled\"><span class=\"toggle-switch toggle-switch-disabled\"><span class=\"toggle-knob toggle-knob-off\"></span></span>POCSAG (N/A)</span>';";
  html += "  html += '</div></div>';";
  html += "  return html;";
  html += "}";
  html += "function refreshActivity() {";
  html += "  let activeCallsigns = [];";
  html += "  fetch('/api/dmr-slot1').then(r => r.json()).then(data => {";
  html += "    document.getElementById('dmr-activity-slot1').innerHTML = renderDMRSlot(data);";
  html += "    if (data.active && data.srcCallsign) activeCallsigns.push(data.srcCallsign);";
  html += "    updateTitle(activeCallsigns);";
  html += "  });";
  html += "  fetch('/api/dmr-slot2').then(r => r.json()).then(data => {";
  html += "    document.getElementById('dmr-activity-slot2').innerHTML = renderDMRSlot(data);";
  html += "    if (data.active && data.srcCallsign) activeCallsigns.push(data.srcCallsign);";
  html += "    updateTitle(activeCallsigns);";
  html += "  });";
  html += "}";
  html += "function updateTitle(callsigns) {";
  html += "  const baseTitle = '" + dmr_callsign + " - ESP32 MMDVM Hotspot';";
  html += "  if (callsigns.length > 0) {";
  html += "    document.title = '>>' + callsigns.join(', ') + '<< - Active';";
  html += "  } else {";
  html += "    document.title = baseTitle;";
  html += "  }";
  html += "}";
  html += "function refreshHistory() {";
  html += "  fetch('/api/dmr-history').then(r => r.json()).then(data => {";
  html += "    document.getElementById('dmr-history-content').innerHTML = renderDMRHistory(data);";
  html += "    filterHistory();";
  html += "  });";
  html += "}";
  html += "function refreshRFHistory() {";
  html += "  fetch('/api/rf-history').then(r => r.json()).then(data => {";
  html += "    document.getElementById('rf-history-content').innerHTML = renderRFHistory(data);";
  html += "  });";
  html += "}";
  html += "function refreshSystemStatus() {";
  html += "  fetch('/api/system-status').then(r => r.json()).then(data => {";
  html += "    document.getElementById('system-status-content').innerHTML = renderSystemStatus(data);";
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
  html += "setInterval(refreshRFHistory, 2000);";
  html += "setInterval(refreshSystemStatus, 3000);";
  html += "</script>";
  html += "</head><body>";
  html += getNavigation("main");
  html += "<div class='container'>";
  html += "<h1><center>" + dmr_callsign + " - ESP32 Dashboard</center></h1>";

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

  // Local RF Activity Card (full width) - Shows outgoing transmissions
  html += "<div class='card history-card'>";
  html += "<div class='history-title-bar'>";
  html += "<h3>Local RF Activity (Last 15 Transmissions)</h3>";
  html += "<div style='font-size: 0.9em; color: var(--text-color); opacity: 0.7;'>Local RF Activity</div>";
  html += "</div>";
  html += "<div id='rf-history-content'>";
  html += getRFHistoryHTML();
  html += "</div>";
  html += "</div>";

  html += "<div class='grid'>";
  html += "<div class='card'>";
  html += "<h3>System Status Overview</h3>";

  // Add custom CSS for status badges
  html += "<style>";
  html += ".status-badges { display: flex; flex-wrap: wrap; gap: 10px; margin: 15px 0; }";
  html += ".status-badge { padding: 8px 16px; border-radius: 20px; font-size: 14px; font-weight: 500; ";
  html += "display: inline-flex; align-items: center; gap: 6px; border: 2px solid; }";
  html += ".badge-active { background: #d4edda; color: #155724; border-color: #28a745; }";
  html += ".badge-inactive { background: #f8d7da; color: #721c24; border-color: #dc3545; }";
  html += ".badge-disabled { background: #e2e3e5; color: #6c757d; border-color: #6c757d; opacity: 0.6; }";
  html += ".badge-warning { background: #fff3cd; color: #856404; border-color: #ffc107; }";
  html += ".status-dot { width: 10px; height: 10px; border-radius: 50%; display: inline-block; }";
  html += ".dot-green { background: #28a745; }";
  html += ".dot-red { background: #dc3545; }";
  html += ".dot-gray { background: #6c757d; }";
  html += ".dot-yellow { background: #ffc107; }";
  // Toggle switch style for mode badges
  html += ".toggle-switch { position: relative; width: 36px; height: 20px; border-radius: 10px; display: inline-block; margin-right: 8px; }";
  html += ".toggle-switch-on { background: #28a745; }";
  html += ".toggle-switch-off { background: #dc3545; }";
  html += ".toggle-switch-disabled { background: #6c757d; opacity: 0.5; }";
  html += ".toggle-knob { position: absolute; top: 2px; width: 16px; height: 16px; border-radius: 50%; background: white; ";
  html += "transition: transform 0.2s; box-shadow: 0 2px 4px rgba(0,0,0,0.2); }";
  html += ".toggle-knob-on { transform: translateX(18px); }";
  html += ".toggle-knob-off { transform: translateX(2px); }";
  html += ".mode-badge { padding: 8px 16px; border-radius: 20px; font-size: 14px; font-weight: 500; ";
  html += "display: inline-flex; align-items: center; gap: 8px; border: 2px solid; }";
  html += ".mode-on { background: #d4edda; color: #155724; border-color: #28a745; }";
  html += ".mode-off { background: #f8d7da; color: #721c24; border-color: #dc3545; }";
  html += ".mode-disabled { background: #e2e3e5; color: #6c757d; border-color: #6c757d; opacity: 0.6; }";
  html += ".status-section { margin: 10px 0; }";
  html += ".status-section-title { font-weight: bold; margin: 10px 0 5px 0; color: var(--text-color); }";
  html += "</style>";

  // System Status Content - will be auto-refreshed
  html += "<div id='system-status-content'>";
  html += getSystemStatusHTML();
  html += "</div>";

  html += "</div>";
  html += "</div>";

  html += getFooter();
  html += "</div></body></html>";
  server.send(200, "text/html; charset=UTF-8", html);
}

#endif // WEB_PAGES_HOME_H
