// ── Web server for twatch-espnow ─────────────────────────────────────────────
// Serves a system-info dashboard on port 80 (online mode only).
// WebServer.h must be included in the main .ino before this header.
#pragma once

WebServer webServer(80);

// ── /api/status ───────────────────────────────────────────────────────────────
static const char* resetReasonStr() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  return "Power-on";
    case ESP_RST_EXT:      return "External";
    case ESP_RST_SW:       return "Software";
    case ESP_RST_PANIC:    return "Panic";
    case ESP_RST_INT_WDT:  return "Int WDT";
    case ESP_RST_TASK_WDT: return "Task WDT";
    case ESP_RST_WDT:      return "WDT";
    case ESP_RST_DEEPSLEEP:return "Deep Sleep";
    case ESP_RST_BROWNOUT: return "Brownout";
    case ESP_RST_SDIO:     return "SDIO";
    default:               return "Unknown";
  }
}

static void handleApiStatus() {
  unsigned long up = millis() / 1000;
  char upStr[16];
  snprintf(upStr, sizeof(upStr), "%02lu:%02lu:%02lu", up/3600, (up%3600)/60, up%60);

  uint8_t mac[6];
  char macStr[18];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

  String j = "{";
  // Network
  j += "\"hostname\":\"twatch-espnow\",";
  j += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  j += "\"mac\":\"" + String(macStr) + "\",";
  j += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  j += "\"ssid\":\"" + WiFi.SSID() + "\",";
  j += "\"channel\":" + String(WiFi.channel()) + ",";
  // Runtime
  j += "\"uptime\":\"" + String(upStr) + "\",";
  j += "\"mode\":\"" + String(onlineMode ? "online" : "offline") + "\",";
  j += "\"synced\":" + String(espnowSynced ? "true" : "false") + ",";
  // Heap
  j += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
  j += "\"totalHeap\":" + String(ESP.getHeapSize()) + ",";
  j += "\"minHeap\":" + String(ESP.getMinFreeHeap()) + ",";
  // Sketch
  j += "\"sketchSize\":" + String(ESP.getSketchSize()) + ",";
  j += "\"sketchFree\":" + String(ESP.getFreeSketchSpace()) + ",";
  // Chip
  j += "\"chipModel\":\"" + String(ESP.getChipModel()) + "\",";
  j += "\"chipRev\":" + String(ESP.getChipRevision()) + ",";
  j += "\"cpuMhz\":" + String(ESP.getCpuFreqMHz()) + ",";
  j += "\"flashSize\":" + String(ESP.getFlashChipSize()) + ",";
  j += "\"sdkVersion\":\"" + String(ESP.getSdkVersion()) + "\",";
  j += "\"resetReason\":\"" + String(resetReasonStr()) + "\",";

  // Battery / power (AXP202)
  bool battConn = ttgo->power->isBatteryConnect();
  bool vbus     = ttgo->power->isVBUSPlug();
  j += "\"battConnected\":" + String(battConn ? "true" : "false") + ",";
  j += "\"vbusConnected\":" + String(vbus     ? "true" : "false") + ",";
  if (battConn) {
    bool charging = ttgo->power->isChargeing();
    j += "\"battPct\":"      + String((int)ttgo->power->getBattPercentage())  + ",";
    j += "\"battVoltage\":"  + String((int)ttgo->power->getBattVoltage())     + ",";
    j += "\"battCharging\":" + String(charging ? "true" : "false")            + ",";
    j += "\"battCurrent\":"  + String((int)(charging
                                 ? ttgo->power->getBattChargeCurrent()
                                 : ttgo->power->getBattDischargeCurrent()))   + ",";
  }
  if (vbus) {
    j += "\"vbusVoltage\":" + String((int)ttgo->power->getVbusVoltage()) + ",";
    j += "\"vbusCurrent\":" + String((int)ttgo->power->getVbusCurrent()) + ",";
  }

  if (lastDmrSrc != 0) {
    j += "\"dmrSrc\":" + String(lastDmrSrc) + ",";
    j += "\"dmrDst\":" + String(lastDmrDst) + ",";
    j += "\"dmrSlot\":" + String(lastDmrSlot) + ",";
    j += "\"dmrGroup\":" + String(lastDmrGroup ? "true" : "false") + ",";
  }
  if (lastRic != 0) {
    String msg = String(lastMsg);
    msg.replace("\\", "\\\\");
    msg.replace("\"", "\\\"");
    j += "\"pagRic\":" + String(lastRic) + ",";
    j += "\"pagMsg\":\"" + msg + "\",";
  }

  // POCSAG log — last 5 messages, newest first
  j += "\"pocLog\":[";
  for (int i = 0; i < pocLogCount; i++) {
    int idx = ((pocLogHead - 1 - i) % 5 + 5) % 5;
    if (i > 0) j += ",";
    String lm = String(pocLog[idx].msg);
    lm.replace("\\", "\\\\");
    lm.replace("\"", "\\\"");
    j += "{\"ric\":" + String(pocLog[idx].ric) + ",";
    j += "\"msg\":\"" + lm + "\",";
    j += "\"time\":\"" + String(pocLog[idx].timeStr) + "\"}";
  }
  j += "],";

  struct tm t;
  if (getClockTime(&t)) {
    char ts[24];
    snprintf(ts, sizeof(ts), "%04d-%02d-%02d %02d:%02d:%02d",
             t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    j += "\"time\":\"" + String(ts) + "\",";
  }

  j += "\"ok\":true}";
  webServer.send(200, "application/json", j);
}

// ── /api/config — GET current config ─────────────────────────────────────────
static void handleApiConfigGet() {
  String j = "{";
  j += "\"hiddenRics\":\"" + hiddenRicsToStr() + "\",";
  j += "\"timeRic\":" + String(timeRic) + ",";
  j += "\"hostname\":\"" + deviceHostname + "\",";
  j += "\"watchface\":" + String(watchfaceId) + ",";
  j += "\"clock24h\":" + String(clock24h ? "true" : "false") + ",";
  j += "\"ok\":true}";
  webServer.send(200, "application/json", j);
}

// ── /api/config/time-ric — POST new time RIC ──────────────────────────────────
static void handleApiConfigSaveTimeRic() {
  String val = webServer.arg("plain");
  val.trim();
  long v = val.toInt();
  if (v <= 0) {
    webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid\"}");
    return;
  }
  saveTimeRic((uint32_t)v);
  Serial.printf("[config] timeRic saved: %u\n", timeRic);
  webServer.send(200, "application/json", "{\"ok\":true}");
}

// ── /api/config/hostname — POST new hostname ───────────────────────────────────
static void handleApiConfigSaveHostname() {
  String val = webServer.arg("plain");
  val.trim();
  if (val.length() == 0 || val.length() > 32) {
    webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid\"}");
    return;
  }
  saveHostname(val);
  Serial.printf("[config] hostname saved: %s\n", deviceHostname.c_str());
  webServer.send(200, "application/json", "{\"ok\":true}");
}

// ── /api/config/watchface — POST 0 or 1 ─────────────────────────────────────
static void handleApiConfigSaveWatchface() {
  String val = webServer.arg("plain");
  val.trim();
  if (val != "0" && val != "1") {
    webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid\"}");
    return;
  }

  uint8_t id = (uint8_t)val.toInt();
  saveWatchfaceSettings(id, clock24h);
  Serial.printf("[config] watchface saved: %u\n", watchfaceId);
  webServer.send(200, "application/json", "{\"ok\":true}");
}

// ── /api/config/clock24h — POST bool (1/0/true/false/24/12) ────────────────
static void handleApiConfigSaveClock24h() {
  String val = webServer.arg("plain");
  val.trim();
  val.toLowerCase();

  bool use24 = false;
  bool valid = true;
  if (val == "1" || val == "true" || val == "on" || val == "24") {
    use24 = true;
  } else if (val == "0" || val == "false" || val == "off" || val == "12") {
    use24 = false;
  } else {
    valid = false;
  }

  if (!valid) {
    webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid\"}");
    return;
  }

  saveWatchfaceSettings(watchfaceId, use24);
  Serial.printf("[config] clock24h saved: %s\n", clock24h ? "true" : "false");
  webServer.send(200, "application/json", "{\"ok\":true}");
}

// ── /api/config/hidden-rics — POST new comma-separated list ──────────────────
static void handleApiConfigSaveRics() {
  String csv = webServer.arg("plain");
  csv.trim();
  if (csv.length() == 0) {
    webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"empty\"}");
    return;
  }
  saveHiddenRics(csv);
  Serial.printf("[config] hiddenRics saved: %s\n", hiddenRicsToStr().c_str());
  webServer.send(200, "application/json", "{\"ok\":true}");
}

// ── / — dashboard page ────────────────────────────────────────────────────────
static void handleRoot() {
  String h;

  // ── head & styles ──────────────────────────────────────────────────────────
  h += "<!DOCTYPE html><html data-theme='dark'><head>";
  h += "<meta charset='UTF-8'>";
  h += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  h += "<title>twatch-espnow</title><style>";
  h += ":root{--bg:#1a1a1a;--card:#2d2d2d;--border:#444;--text:#fff;--muted:#aaa;--accent:#4da6ff;--green:#28a745;--red:#dc3545;}";
  h += "[data-theme='light']{--bg:#f0f0f0;--card:#fff;--border:#ddd;--text:#333;--muted:#666;--accent:#007bff;}";
  h += "*{box-sizing:border-box;margin:0;padding:0;}";
  h += "body{font-family:Arial,sans-serif;background:var(--bg);color:var(--text);padding-top:56px;}";
  h += "nav{position:fixed;top:0;left:0;right:0;height:56px;background:#111;display:flex;align-items:center;padding:0 16px;gap:12px;z-index:100;}";
  h += ".brand{font-weight:bold;font-size:1.1em;color:#fff;}";
  h += ".sub{font-size:.8em;color:#aaa;}";
  h += ".tgl{margin-left:auto;background:#333;border:none;color:#fff;padding:6px 14px;border-radius:20px;cursor:pointer;font-size:.85em;}";
  h += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:16px;padding:16px;}";
  h += ".card{background:var(--card);border:1px solid var(--border);border-radius:8px;padding:16px;}";
  h += ".card h3{font-size:.85em;text-transform:uppercase;color:var(--accent);margin-bottom:12px;letter-spacing:.05em;}";
  h += ".row{display:flex;justify-content:space-between;padding:6px 0;border-bottom:1px solid var(--border);font-size:.9em;}";
  h += ".row:last-child{border-bottom:none;}";
  h += ".lbl{color:var(--muted);white-space:nowrap;padding-right:8px;}";
  h += ".val{font-family:monospace;text-align:right;word-break:break-all;}";
  h += ".badge{padding:2px 8px;border-radius:10px;font-size:.8em;font-weight:bold;}";
  h += ".ok{background:#28a745;color:#fff;} .err{background:#dc3545;color:#fff;} .warn{background:#ffc107;color:#333;} .secondary{background:#6c757d;color:#fff;}";
  h += ".none{color:var(--muted);font-size:.9em;}";
  h += "input[type=text]{background:var(--card);color:var(--text);border:1px solid var(--border);padding:6px 8px;border-radius:4px;width:100%;font-family:monospace;font-size:.9em;box-sizing:border-box;}";
  h += "select{background:var(--card);color:var(--text);border:1px solid var(--border);padding:6px 8px;border-radius:4px;width:100%;font-size:.9em;box-sizing:border-box;}";
  h += "button.save{margin-top:8px;padding:6px 16px;background:var(--accent);color:#fff;border:none;border-radius:4px;cursor:pointer;font-size:.85em;font-weight:bold;}";
  h += "button.save:hover{opacity:.85;}";
  h += ".msg{font-size:.8em;margin-top:6px;min-height:1em;background:none!important;}";
  h += ".msg.ok{color:#28a745;} .msg.err{color:#dc3545;}";
  h += "</style></head><body>";

  // ── navbar ─────────────────────────────────────────────────────────────────
  h += "<nav>";
  h += "<span class='brand'>twatch-espnow</span>";
  h += "<span class='sub' id='nav-ip'>-</span>";
  h += "<button class='tgl' onclick='toggleTheme()'>Light / Dark</button>";
  h += "</nav>";

  // ── cards ──────────────────────────────────────────────────────────────────
  h += "<div class='grid'>";

  // System card
  h += "<div class='card'><h3>Network</h3>";
  h += "<div class='row'><span class='lbl'>Hostname</span><span class='val' id='s-host'>-</span></div>";
  h += "<div class='row'><span class='lbl'>IP Address</span><span class='val' id='s-ip'>-</span></div>";
  h += "<div class='row'><span class='lbl'>MAC</span><span class='val' id='s-mac'>-</span></div>";
  h += "<div class='row'><span class='lbl'>SSID</span><span class='val' id='s-ssid'>-</span></div>";
  h += "<div class='row'><span class='lbl'>WiFi Signal</span><span class='val' id='s-rssi'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Channel</span><span class='val' id='s-ch'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Mode</span><span class='val' id='s-mode'>-</span></div>";
  h += "</div>";

  // Hardware card
  h += "<div class='card'><h3>Hardware</h3>";
  h += "<div class='row'><span class='lbl'>Chip</span><span class='val' id='h-chip'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Revision</span><span class='val' id='h-rev'>-</span></div>";
  h += "<div class='row'><span class='lbl'>CPU</span><span class='val' id='h-cpu'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Flash</span><span class='val' id='h-flash'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Sketch used</span><span class='val' id='h-sketch'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Sketch free</span><span class='val' id='h-sfree'>-</span></div>";
  h += "<div class='row'><span class='lbl'>SDK</span><span class='val' id='h-sdk'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Reset reason</span><span class='val' id='h-rst'>-</span></div>";
  h += "</div>";

  // Runtime card
  h += "<div class='card'><h3>Runtime</h3>";
  h += "<div class='row'><span class='lbl'>Uptime</span><span class='val' id='s-up'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Free heap</span><span class='val' id='s-heap'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Total heap</span><span class='val' id='s-theap'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Min free heap</span><span class='val' id='s-mheap'>-</span></div>";
  h += "</div>";

  // Clock card
  h += "<div class='card'><h3>Clock</h3>";
  h += "<div class='row'><span class='lbl'>Time</span><span class='val' id='c-time'>-</span></div>";
  h += "<div class='row'><span class='lbl'>ESP-NOW sync</span><span class='val' id='c-sync'>-</span></div>";
  h += "</div>";

  // Battery card
  h += "<div class='card'><h3>Battery</h3>";
  h += "<div class='row'><span class='lbl'>Battery</span><span class='val' id='b-conn'>-</span></div>";
  h += "<div class='row' id='b-rows' style='display:none;flex-direction:column;padding:0'>";
  h += "<div class='row'><span class='lbl'>Charge</span><span class='val' id='b-pct'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Voltage</span><span class='val' id='b-volt'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Status</span><span class='val' id='b-status'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Current</span><span class='val' id='b-cur'>-</span></div>";
  h += "</div>";
  h += "<div class='row'><span class='lbl'>USB</span><span class='val' id='b-vbus'>-</span></div>";
  h += "<div class='row' id='b-vbus-rows' style='display:none;flex-direction:column;padding:0'>";
  h += "<div class='row'><span class='lbl'>USB Voltage</span><span class='val' id='b-vvolt'>-</span></div>";
  h += "<div class='row'><span class='lbl'>USB Current</span><span class='val' id='b-vcur'>-</span></div>";
  h += "</div>";
  h += "</div>";

  // DMR card
  h += "<div class='card'><h3>Last DMR</h3>";
  h += "<div id='d-none' class='none'>No packets yet</div>";
  h += "<div id='d-data' style='display:none'>";
  h += "<div class='row'><span class='lbl'>Source</span><span class='val' id='d-src'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Destination</span><span class='val' id='d-dst'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Slot</span><span class='val' id='d-slot'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Type</span><span class='val' id='d-type'>-</span></div>";
  h += "</div></div>";

  // POCSAG card
  h += "<div class='card'><h3>Last POCSAG</h3>";
  h += "<div id='p-none' class='none'>No packets yet</div>";
  h += "<div id='p-data' style='display:none'>";
  h += "<div class='row'><span class='lbl'>RIC</span><span class='val' id='p-ric'>-</span></div>";
  h += "<div class='row'><span class='lbl'>Message</span><span class='val' id='p-msg' style='font-family:sans-serif;text-align:left'>-</span></div>";
  h += "</div></div>";

  // Recent Messages card
  h += "<div class='card'><h3>Recent Messages</h3>";
  h += "<div id='log-none' class='none'>No messages yet</div>";
  h += "<div id='log-list'></div>";
  h += "</div>";

  // Config card
  h += "<div class='card'><h3>Config</h3>";
  h += "<div class='row'><span class='lbl'>Time RIC</span>";
  h += "<input type='text' id='cfg-tric' placeholder='224' style='max-width:100px'></div>";
  h += "<div class='row'><span class='lbl'>Hostname</span>";
  h += "<input type='text' id='cfg-host' placeholder='twatch-espnow'></div>";
  h += "<div class='row'><span class='lbl'>Hidden RICs</span>";
  h += "<input type='text' id='cfg-rics' placeholder='224,208,200,216'></div>";
  h += "<button class='save' style='margin-top:10px;width:100%' onclick='saveAllConfig()'>Save</button>";
  h += "<div class='msg' id='cfg-msg'></div>";
  h += "</div>";

  // Watchface card
  h += "<div class='card'><h3>Watchface</h3>";
  h += "<div class='row'><span class='lbl'>Layout</span>";
  h += "<select id='wf-layout' style='max-width:140px'>";
  h += "<option value='0'>Analog</option>";
  h += "<option value='1'>Digital</option>";
  h += "</select></div>";
  h += "<div class='row'><span class='lbl'>Clock Format</span>";
  h += "<select id='wf-format' style='max-width:140px'>";
  h += "<option value='12'>12H (AM/PM)</option>";
  h += "<option value='24'>24H</option>";
  h += "</select></div>";
  h += "<button class='save' style='margin-top:10px;width:100%' onclick='saveWatchfaceConfig()'>Save</button>";
  h += "<div class='msg' id='wf-msg'></div>";
  h += "</div>";

  h += "</div>"; // .grid

  // ── script ─────────────────────────────────────────────────────────────────
  h += "<script>";
  h += "function toggleTheme(){";
  h += "var el=document.documentElement;";
  h += "el.dataset.theme=el.dataset.theme==='dark'?'light':'dark';}";

  h += "function fmt(n){";
  h += "if(n>=1048576)return (n/1048576).toFixed(1)+' MB';";
  h += "if(n>=1024)return (n/1024).toFixed(1)+' KB';";
  h += "return n+' B';}";

  h += "function badge(cls,txt){return '<span class=\"badge '+cls+'\">'+txt+'</span>';}";

  h += "function rssiBar(r){";
  h += "var pct=Math.min(100,Math.max(0,2*(r+100)));";
  h += "var col=pct>60?'#28a745':pct>30?'#ffc107':'#dc3545';";
  h += "return r+' dBm <span style=\"display:inline-block;width:60px;height:10px;background:#333;border-radius:5px;vertical-align:middle;margin-left:4px\"><span style=\"display:block;width:'+pct+'%;height:100%;background:'+col+';border-radius:5px\"></span></span>';}";

  h += "function poll(){fetch('/api/status').then(function(r){return r.json();}).then(function(d){";
  // Network
  h += "document.getElementById('nav-ip').textContent=d.ip||'-';";
  h += "document.getElementById('s-host').textContent=d.hostname||'-';";
  h += "document.getElementById('s-ip').textContent=d.ip||'-';";
  h += "document.getElementById('s-mac').textContent=d.mac||'-';";
  h += "document.getElementById('s-ssid').textContent=d.ssid||'-';";
  h += "document.getElementById('s-rssi').innerHTML=d.rssi?rssiBar(d.rssi):'-';";
  h += "document.getElementById('s-ch').textContent=d.channel||'-';";
  h += "document.getElementById('s-mode').innerHTML=badge(d.mode==='online'?'ok':'warn',d.mode||'-');";
  // Hardware
  h += "document.getElementById('h-chip').textContent=d.chipModel||'-';";
  h += "document.getElementById('h-rev').textContent='v'+d.chipRev;";
  h += "document.getElementById('h-cpu').textContent=d.cpuMhz+' MHz';";
  h += "document.getElementById('h-flash').textContent=fmt(d.flashSize||0);";
  h += "document.getElementById('h-sketch').textContent=fmt(d.sketchSize||0);";
  h += "document.getElementById('h-sfree').textContent=fmt(d.sketchFree||0);";
  h += "document.getElementById('h-sdk').textContent=d.sdkVersion||'-';";
  h += "document.getElementById('h-rst').textContent=d.resetReason||'-';";
  // Runtime
  h += "document.getElementById('s-up').textContent=d.uptime||'-';";
  h += "document.getElementById('s-heap').textContent=fmt(d.freeHeap||0);";
  h += "document.getElementById('s-theap').textContent=fmt(d.totalHeap||0);";
  h += "document.getElementById('s-mheap').textContent=fmt(d.minHeap||0);";
  // Clock
  h += "document.getElementById('c-time').textContent=d.time||'--';";
  h += "document.getElementById('c-sync').innerHTML=badge(d.synced?'ok':'err',d.synced?'Yes':'No');";
  // Battery
  h += "document.getElementById('b-conn').innerHTML=badge(d.battConnected?'ok':'err',d.battConnected?'Connected':'Not connected');";
  h += "if(d.battConnected){";
  h += "document.getElementById('b-rows').style.display='';";
  h += "var pctCol=d.battPct>50?'#28a745':d.battPct>20?'#ffc107':'#dc3545';";
  h += "document.getElementById('b-pct').innerHTML='<span style=\"color:'+pctCol+';font-weight:bold\">'+d.battPct+'%</span> "
       "<span style=\"display:inline-block;width:60px;height:10px;background:#333;border-radius:5px;vertical-align:middle;margin-left:4px\">"
       "<span style=\"display:block;width:'+d.battPct+'%;height:100%;background:'+pctCol+';border-radius:5px\"></span></span>';";
  h += "document.getElementById('b-volt').textContent=(d.battVoltage||0)+' mV';";
  h += "document.getElementById('b-status').innerHTML=badge(d.battCharging?'ok':'warn',d.battCharging?'Charging':'Discharging');";
  h += "document.getElementById('b-cur').textContent=(d.battCurrent||0)+' mA';}else{document.getElementById('b-rows').style.display='none';}";
  h += "document.getElementById('b-vbus').innerHTML=badge(d.vbusConnected?'ok':'secondary',d.vbusConnected?'Connected':'Disconnected');";
  h += "if(d.vbusConnected){";
  h += "document.getElementById('b-vbus-rows').style.display='';";
  h += "document.getElementById('b-vvolt').textContent=(d.vbusVoltage||0)+' mV';";
  h += "document.getElementById('b-vcur').textContent=(d.vbusCurrent||0)+' mA';}else{document.getElementById('b-vbus-rows').style.display='none';}";
  // DMR
  h += "if(d.dmrSrc){";
  h += "document.getElementById('d-none').style.display='none';";
  h += "document.getElementById('d-data').style.display='';";
  h += "document.getElementById('d-src').textContent=d.dmrSrc;";
  h += "document.getElementById('d-dst').textContent=d.dmrDst;";
  h += "document.getElementById('d-slot').textContent='TS'+d.dmrSlot;";
  h += "document.getElementById('d-type').textContent=d.dmrGroup?'TalkGroup':'Private';}";
  // POCSAG last shown on screen
  h += "if(d.pagRic){";
  h += "document.getElementById('p-none').style.display='none';";
  h += "document.getElementById('p-data').style.display='';";
  h += "document.getElementById('p-ric').textContent=d.pagRic;";
  h += "document.getElementById('p-msg').textContent=d.pagMsg||'';}";
  // Recent messages log
  h += "var log=d.pocLog||[];";
  h += "var logEl=document.getElementById('log-list');";
  h += "var noneEl=document.getElementById('log-none');";
  h += "if(log.length===0){noneEl.style.display='';logEl.innerHTML='';}else{";
  h += "noneEl.style.display='none';";
  h += "var html='';";
  h += "for(var i=0;i<log.length;i++){";
  h += "var entry=log[i];";
  h += "html+='<div class=\"row\" style=\"flex-direction:column;align-items:flex-start;padding:8px 0\">';";
  h += "html+='<div style=\"display:flex;justify-content:space-between;width:100%;margin-bottom:4px\">';";
  h += "html+='<span class=\"lbl\">RIC '+entry.ric+'</span>';";
  h += "html+='<span style=\"font-size:.8em;color:var(--muted)\">'+entry.time+'</span>';";
  h += "html+='</div>';";
  h += "html+='<span style=\"font-size:.9em;word-break:break-word\">'+entry.msg+'</span>';";
  h += "html+='</div>';}";
  h += "logEl.innerHTML=html;}";
  h += "}).catch(function(){});}";

  h += "function loadConfig(){fetch('/api/config').then(function(r){return r.json();}).then(function(d){";
  h += "document.getElementById('cfg-tric').value=d.timeRic||'';";
  h += "document.getElementById('cfg-host').value=d.hostname||'';";
  h += "document.getElementById('cfg-rics').value=d.hiddenRics||'';";
  h += "document.getElementById('wf-layout').value=(d.watchface===1||d.watchface==='1')?'1':'0';";
  h += "document.getElementById('wf-format').value=d.clock24h?'24':'12';";
  h += "}).catch(function(){});}";

  h += "function saveAllConfig(){";
  h += "var tric=document.getElementById('cfg-tric').value.trim();";
  h += "var host=document.getElementById('cfg-host').value.trim();";
  h += "var rics=document.getElementById('cfg-rics').value.trim();";
  h += "var m=document.getElementById('cfg-msg');";
  h += "m.className='msg';m.textContent='Saving...';";
  h += "Promise.all([";
  h += "fetch('/api/config/time-ric',{method:'POST',body:tric}).then(function(r){return r.json();}),";
  h += "fetch('/api/config/hostname',{method:'POST',body:host}).then(function(r){return r.json();}),";
  h += "fetch('/api/config/hidden-rics',{method:'POST',body:rics}).then(function(r){return r.json();})";
  h += "]).then(function(results){";
  h += "var ok=results.every(function(d){return d.ok;});";
  h += "m.className='msg '+(ok?'ok':'err');";
  h += "m.textContent=ok?'Saved!':'Error saving one or more fields';";
  h += "}).catch(function(){m.className='msg err';m.textContent='Request failed';});}";

  h += "function saveWatchfaceConfig(){";
  h += "var layout=document.getElementById('wf-layout').value;";
  h += "var fmt=document.getElementById('wf-format').value;";
  h += "var m=document.getElementById('wf-msg');";
  h += "m.className='msg';m.textContent='Saving...';";
  h += "fetch('/api/config/watchface',{method:'POST',body:layout}).then(function(r){return r.json();}).then(function(a){";
  h += "if(!a.ok){throw new Error('watchface');}";
  h += "return fetch('/api/config/clock24h',{method:'POST',body:(fmt==='24'?'1':'0')}).then(function(r){return r.json();});";
  h += "}).then(function(b){";
  h += "var ok=!!b.ok;";
  h += "m.className='msg '+(ok?'ok':'err');";
  h += "m.textContent=ok?'Saved!':'Error saving watchface settings';";
  h += "if(ok){loadConfig();}";
  h += "}).catch(function(){m.className='msg err';m.textContent='Request failed';});}";

  h += "poll();setInterval(poll,3000);loadConfig();";
  h += "</script></body></html>";

  webServer.send(200, "text/html", h);
}

// ── Call from setup() after WiFi connects ─────────────────────────────────────
void setupWebServer() {
  webServer.on("/",                       handleRoot);
  webServer.on("/api/status",             handleApiStatus);
  webServer.on("/api/config",             HTTP_GET,  handleApiConfigGet);
  webServer.on("/api/config/hidden-rics", HTTP_POST, handleApiConfigSaveRics);
  webServer.on("/api/config/time-ric",    HTTP_POST, handleApiConfigSaveTimeRic);
  webServer.on("/api/config/hostname",    HTTP_POST, handleApiConfigSaveHostname);
  webServer.on("/api/config/watchface",   HTTP_POST, handleApiConfigSaveWatchface);
  webServer.on("/api/config/clock24h",    HTTP_POST, handleApiConfigSaveClock24h);
  webServer.begin();
  Serial.println("Web server started — http://twatch-espnow.local/");
}
