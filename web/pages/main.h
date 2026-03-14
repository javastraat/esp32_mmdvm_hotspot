/*
 * Main Web Page HTML
 * Separate HTML content for easier maintenance and adding more pages
 */

#ifndef WEB_MAIN_H
#define WEB_MAIN_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"
#include "web/include/country_codes.h"

// Generate the main page HTML
String getMainPageHTML(int requestCount, float cpuUsage, uint32_t heapSize)
{
  extern String userCallsign;
  extern String mdnsHostname;
  extern bool modePocsagEnabled;
  extern bool dapnetEnabled;
  extern bool modeDmrEnabled, modeDstarEnabled, modeYsfEnabled;
  extern bool modeP25Enabled, modeNxdnEnabled;
  bool anyVoiceModeEnabled = modeDmrEnabled || modeDstarEnabled || modeYsfEnabled || modeP25Enabled || modeNxdnEnabled;
  String html;
  html.reserve(59000);
  html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>" + userCallsign + " - " + mdnsHostname + "</title>";

  // Add shared styles
  html += getSharedStyles();

  // Page-specific styles
  html += "<style>";
  html += ".hero{text-align:center;padding:30px 20px 20px 20px}";
  html += ".hero h1{font-size:2.5em;margin-bottom:20px;color:var(--text-primary)}";
  html += ".hero p{font-size:1.2em;color:var(--text-secondary);margin-bottom:40px}";
  html += ".feature{background:var(--bg-secondary);padding:30px;border-radius:10px;border:1px solid var(--border-color)}";
  html += ".feature h3{color:var(--text-primary);margin-top:0}";
  html += ".feature p{color:var(--text-secondary)}";
  html += "</style>";
  html += "</head><body>";

  html += getNavigation("home");

  html += "<div class='container'>";
  html += "<div class='hero'>";
  html += "<h1>" + userCallsign + " - " + mdnsHostname + "</h1>";

  // Page for N0CALL with no callsign set up yet
  if (userCallsign == "N0CALL") {
    html += "<p style='color:var(--text-secondary);margin-bottom:0;'>Multi-mode digital voice hotspot</p>";
    html += "</div>"; // end hero
    html += "<div style='display:flex;flex-direction:column;gap:20px;margin-bottom:30px;'>";
    html += "<div class='feature'>";
    html += "<h3 style='margin-top:0;'>Welcome to ESP32 MMDVM</h3>";
    html += "<p style='color:var(--text-secondary);margin-bottom:16px;'>This is an <strong style='color:var(--text-primary);'>ESP32-based MMDVM hotspot</strong> for amateur radio operators. It connects your station to digital radio networks and amateur paging systems over the internet &mdash; all configured and monitored from this web interface.</p>";
    html += "<p style='color:var(--text-secondary);font-size:0.85em;font-weight:bold;margin-bottom:8px;letter-spacing:0.05em;text-transform:uppercase;'>Digital Voice &amp; Paging</p>";
    html += "<div style='display:flex;flex-direction:column;gap:8px;margin-bottom:16px;'>";
    html += "<div style='display:flex;gap:12px;align-items:flex-start;'><span style='color:#28a745;font-size:1.1em;flex-shrink:0;'>&#9679;</span><div><strong style='color:var(--text-primary);'>DMR &mdash; Digital Mobile Radio</strong><br><span style='color:var(--text-secondary);font-size:0.9em;'>Connect to worldwide talkgroups via BrandMeister or DMR+ networks. Talk to amateur radio operators around the globe with your DMR radio.</span></div></div>";
    html += "<div style='display:flex;gap:12px;align-items:flex-start;'><span style='color:#28a745;font-size:1.1em;flex-shrink:0;'>&#9679;</span><div><strong style='color:var(--text-primary);'>DAPNET &mdash; Decentralised Amateur Paging Network</strong><br><span style='color:var(--text-secondary);font-size:0.9em;'>Receive paging messages from the DAPNET network, used by amateur radio operators and emergency services across Europe and beyond.</span></div></div>";
    html += "<div style='display:flex;gap:12px;align-items:flex-start;'><span style='color:#28a745;font-size:1.1em;flex-shrink:0;'>&#9679;</span><div><strong style='color:var(--text-primary);'>POCSAG &mdash; RF Pager Transmitter</strong><br><span style='color:var(--text-secondary);font-size:0.9em;'>Transmit DAPNET messages over the air via POCSAG to compatible pager receivers in your area &mdash; turning this hotspot into a local POCSAG transmitter (with API support, no DAPNET needed).</span></div></div>";
    html += "</div>";
    html += "<p style='color:var(--text-secondary);font-size:0.85em;font-weight:bold;margin-bottom:8px;letter-spacing:0.05em;text-transform:uppercase;'>Connectivity &amp; Integration</p>";
    html += "<div style='display:flex;flex-direction:column;gap:8px;margin-bottom:16px;'>";
    html += "<div style='display:flex;gap:12px;align-items:flex-start;'><span style='color:#28a745;font-size:1.1em;flex-shrink:0;'>&#9679;</span><div><strong style='color:var(--text-primary);'>MQTT</strong><br><span style='color:var(--text-secondary);font-size:0.9em;'>Publish DMR activity, POCSAG messages, and system events to any MQTT broker. Integrate with Home Assistant, Node-RED, or your own automation.</span></div></div>";
    html += "<div style='display:flex;gap:12px;align-items:flex-start;'><span style='color:#28a745;font-size:1.1em;flex-shrink:0;'>&#9679;</span><div><strong style='color:var(--text-primary);'>WireGuard VPN</strong><br><span style='color:var(--text-secondary);font-size:0.9em;'>Built-in WireGuard client. Securely connect your hotspot to a private network or access it remotely without port forwarding.</span></div></div>";
    html += "<div style='display:flex;gap:12px;align-items:flex-start;'><span style='color:#28a745;font-size:1.1em;flex-shrink:0;'>&#9679;</span><div><strong style='color:var(--text-primary);'>WiFi &amp; Ethernet</strong><br><span style='color:var(--text-secondary);font-size:0.9em;'>Connect via WiFi or wired Ethernet. Supports mDNS for easy local hostname access.</span></div></div>";
    html += "<div style='display:flex;gap:12px;align-items:flex-start;'><span style='color:#28a745;font-size:1.1em;flex-shrink:0;'>&#9679;</span><div><strong style='color:var(--text-primary);'>OLED Display</strong><br><span style='color:var(--text-secondary);font-size:0.9em;'>Optional OLED screen shows live call info, callsign, and system status. Preview the display in real time from this web interface.</span></div></div>";
    html += "<div style='display:flex;gap:12px;align-items:flex-start;'><span style='color:#28a745;font-size:1.1em;flex-shrink:0;'>&#9679;</span><div><strong style='color:var(--text-primary);'>SD Card &mdash; DMR User Database</strong><br><span style='color:var(--text-secondary);font-size:0.9em;'>Store the full DMR user database on a microSD card for fast local callsign lookups. Users are resolved from an in-memory cache first, then from the SD card, with a fallback to RadioID.net for any missing entries.</span></div></div>";
    html += "</div>";
    html += "<p style='color:var(--text-secondary);font-size:0.85em;font-weight:bold;margin-bottom:8px;letter-spacing:0.05em;text-transform:uppercase;'>Firmware Updates</p>";
    html += "<div style='display:flex;flex-direction:column;gap:8px;margin-bottom:4px;'>";
    html += "<div style='display:flex;gap:12px;align-items:flex-start;'><span style='color:#28a745;font-size:1.1em;flex-shrink:0;'>&#9679;</span><div><strong style='color:var(--text-primary);'>ESP32 OTA &mdash; Over-the-Air Updates</strong><br><span style='color:var(--text-secondary);font-size:0.9em;'>Update the ESP32 firmware directly from this web interface or via Arduino IDE OTA &mdash; no USB cable, no disassembly needed.</span></div></div>";
    html += "<div style='display:flex;gap:12px;align-items:flex-start;'><span style='color:#28a745;font-size:1.1em;flex-shrink:0;'>&#9679;</span><div><strong style='color:var(--text-primary);'>Modem Firmware OTA</strong><br><span style='color:var(--text-secondary);font-size:0.9em;'>Flash the MMDVM modem firmware over the air directly from the web interface, keeping your modem up to date without any tools.</span></div></div>";
    html += "</div>";
    html += "</div>";
    html += "<div class='feature'>";
    html += "<h3 style='margin-top:0;color:#dc3545;'>&#9888; First-time Setup</h3>";
    html += "<p style='color:var(--text-secondary);margin-bottom:16px;'>Enter your callsign and DMR ID to get started. All other settings can be configured from the menu afterwards.</p>";
    html += "<div style='display:flex;flex-direction:column;gap:12px;max-width:320px;'>";
    html += "<div><label style='display:block;font-size:0.85em;color:var(--text-secondary);margin-bottom:4px;'>Callsign</label>";
    html += "<input id='setup-callsign' type='text' maxlength='10' placeholder='e.g. N0CALL' style='width:100%;box-sizing:border-box;background:var(--bg-primary);color:var(--text-primary);border:1px solid var(--border-color);border-radius:6px;padding:8px 10px;font-size:1em;text-transform:uppercase;'></div>";
    html += "<div><label style='display:block;font-size:0.85em;color:var(--text-secondary);margin-bottom:4px;'>DMR ID</label>";
    html += "<input id='setup-dmrid' type='number' min='1' max='9999999' placeholder='e.g. 1234567' style='width:100%;box-sizing:border-box;background:var(--bg-primary);color:var(--text-primary);border:1px solid var(--border-color);border-radius:6px;padding:8px 10px;font-size:1em;'></div>";
    html += "<div id='setup-msg' style='font-size:0.85em;min-height:1.2em;'></div>";
    html += "<button onclick='saveSetup()' style='background:#28a745;color:#fff;border:none;border-radius:6px;padding:10px 20px;font-size:1em;font-weight:bold;cursor:pointer;width:fit-content;'>Save &amp; Continue &#8594;</button>";
    html += "</div></div>";
    html += "</div>"; // end setup cards
    html += "<script>";
    html += "document.getElementById('setup-callsign').addEventListener('input',function(){ this.value=this.value.toUpperCase(); });";
    html += "function saveSetup(){";
    html += "  var cs=document.getElementById('setup-callsign').value.trim().toUpperCase();";
    html += "  var id=document.getElementById('setup-dmrid').value.trim();";
    html += "  var msg=document.getElementById('setup-msg');";
    html += "  if(cs.length<3||cs.length>10){ msg.style.color='#dc3545'; msg.textContent='Callsign must be 3-10 characters.'; return; }";
    html += "  if(!id||parseInt(id)<1||parseInt(id)>9999999){ msg.style.color='#dc3545'; msg.textContent='Enter a valid DMR ID (1-7 digits).'; return; }";
    html += "  msg.style.color='var(--text-secondary)'; msg.textContent='Saving...';";
    html += "  var fd=new FormData(); fd.append('callsign',cs); fd.append('dmrid',id); fd.append('ssid','0');";
    html += "  fetch('/api/save-station',{method:'POST',body:fd}).then(function(r){ return r.text(); }).then(function(){";
    html += "    msg.style.color='#28a745'; msg.textContent='Saved! Reloading...'; setTimeout(function(){ location.reload(); },1000);";
    html += "  }).catch(function(){ msg.style.color='#dc3545'; msg.textContent='Save failed. Check connection.'; });";
    html += "}";
    html += "</script>";
  } else {
    html += "</div>"; // end hero

    html += "<div style='display:flex;flex-direction:column;gap:20px;margin-bottom:30px;'>";
  //
  // Page after callsign is configured - stacked cards section
  // Stacked cards section
  if (anyVoiceModeEnabled) {
    // Card 1: On Air
    html += "<div class='feature' id='onair-card' style='transition:box-shadow 0.4s,border-color 0.4s;'>";
    html += "<div style='display:flex;align-items:center;margin-bottom:16px;'>";
    html += "<div style='flex:1;'></div>";
    html += "<div id='onair-badge' style='display:inline-block;padding:6px 20px;border-radius:20px;font-size:1.1em;font-weight:bold;background:#555;color:#fff;'>IDLE</div>";
    html += "<div style='flex:1;display:flex;justify-content:flex-end;'><span id='onair-timer' style='font-size:1.1em;font-weight:bold;letter-spacing:1px;'></span></div>";
    html += "</div>";
    html += "<div id='onair-info' style='display:flex;align-items:center;justify-content:center;gap:10px;flex-wrap:wrap;min-height:2em;'></div>";
    html += "</div>";

    // Card 2: Last 15 Calls
    html += "<div class='feature'>";
    html += "<div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;'>";
    html += "<h3 style='margin:0;'>Last 15 Calls</h3>";
    html += "<select id='dur-filter' style='background:var(--bg-primary);color:var(--text-secondary);border:1px solid var(--border-color);border-radius:6px;padding:3px 8px;font-size:0.8em;cursor:pointer;'>";
    html += "<option value='0'>All</option>";
    html += "<option value='1'>&gt; 1s</option>";
    html += "<option value='2'>&gt; 2s</option>";
    html += "<option value='3'>&gt; 3s</option>";
    html += "<option value='4'>&gt; 4s</option>";
    html += "<option value='5'>&gt; 5s</option>";
    html += "</select>";
    html += "</div>";
    html += "<div style='overflow-x:auto;'>";
    html += "<table style='width:100%;border-collapse:collapse;font-size:0.85em;'>";
    html += "<thead><tr style='border-bottom:1px solid var(--border-color);'>";
    html += "<th style='text-align:left;padding:4px 6px;color:var(--text-secondary);'>Time</th>";
    html += "<th style='text-align:left;padding:4px 6px;color:var(--text-secondary);'>Dur.</th>";
    html += "<th style='text-align:left;padding:4px 6px;color:var(--text-secondary);'>Callsign</th>";
    html += "<th style='text-align:left;padding:4px 6px;color:var(--text-secondary);'>Name</th>";
    html += "<th style='text-align:left;padding:4px 6px;color:var(--text-secondary);'>City</th>";
    html += "<th style='text-align:left;padding:4px 6px;color:var(--text-secondary);'>Country</th>";
    html += "</tr></thead>";
    html += "<tbody id='call-history-body'><tr><td colspan='6' style='padding:8px 6px;color:var(--text-secondary);'>No calls yet</td></tr></tbody>";
    html += "</table></div></div>";
  } else if (!dapnetEnabled && !modePocsagEnabled) {
    // No voice mode, no DAPNET, no POCSAG — nothing else will render below, show the notice
    html += "<div class='feature' style='text-align:center;padding:30px 20px;'>";
    html += "<div style='font-size:2.4em;margin-bottom:12px;'>&#128263;</div>";
    html += "<h3 style='margin-top:0;color:var(--text-secondary);'>No Mode Enabled</h3>";
    html += "<p style='color:var(--text-secondary);margin-bottom:20px;'>Enable at least one digital voice mode &mdash; DMR, D-STAR, YSF, P25, or NXDN &mdash; or POCSAG / DAPNET to see live activity here.</p>";
    html += "<button onclick=\"setTimeout(function(){document.getElementById('menuDropdown').classList.add('active');var s=document.querySelectorAll('.dropdown-submenu');s.forEach(function(e){e.classList.remove('active');});for(var i=0;i<s.length;i++){var sp=s[i].querySelector('span');if(sp&&sp.textContent.indexOf('Mode')>=0){s[i].classList.add('active');break;}}},0)\" style='padding:8px 22px;background:#007bff;color:#fff;border:none;border-radius:6px;font-size:0.95em;cursor:pointer;'>Go to Mode Settings</button>";
    html += "</div>";
  }

  if (modePocsagEnabled) {
    // Card 3: Last Received DAPNET Messages (only if DAPNET client is enabled)
    if (dapnetEnabled) {
    html += "<div class='feature'>";
    html += "<div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;'>";
    html += "<h3 style='margin:0;'>Last Received DAPNET</h3>";
    html += "<span id='dapnet-hist-count' style='font-size:0.8em;color:var(--text-secondary);'></span>";
    html += "</div>";
    html += "<div style='overflow-x:auto;'>";
    html += "<table style='width:100%;border-collapse:collapse;font-size:0.85em;'>";
    html += "<thead><tr style='border-bottom:1px solid var(--border-color);'>";
    html += "<th style='text-align:left;padding:4px 6px;color:var(--text-secondary);white-space:nowrap;'>Time</th>";
    html += "<th style='text-align:left;padding:4px 6px;color:var(--text-secondary);'>RIC</th>";
    html += "<th style='text-align:left;padding:4px 6px;color:var(--text-secondary);'>Message</th>";
    html += "</tr></thead>";
    html += "<tbody id='dapnet-hist-body'><tr><td colspan='3' style='padding:8px 6px;color:var(--text-secondary);'>No messages yet</td></tr></tbody>";
    html += "</table></div></div>";
    } // end if (dapnetEnabled)

    // Card 4: POCSAG Queue
    html += "<div class='feature'>";
    html += "<div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;'>";
    html += "<h3 style='margin:0;'>POCSAG Queue</h3>";
    html += "<span id='queue-status' style='font-size:0.8em;color:var(--text-secondary);'>0 / 0</span>";
    html += "</div>";
    html += "<div style='overflow-x:auto;'>";
    html += "<table style='width:100%;border-collapse:collapse;font-size:0.85em;'>";
    html += "<thead><tr style='border-bottom:1px solid var(--border-color);'>";
    html += "<th style='text-align:left;padding:4px 6px;color:var(--text-secondary);'>RIC</th>";
    html += "<th style='text-align:left;padding:4px 6px;color:var(--text-secondary);'>Message</th>";
    html += "</tr></thead>";
    html += "<tbody id='pocsag-queue-body'><tr><td colspan='2' style='padding:8px 6px;color:var(--text-secondary);'>Queue empty</td></tr></tbody>";
    html += "</table></div></div>";

    // Card 4: Last 15 Transmitted POCSAG
    html += "<div class='feature'>";
    html += "<div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;'>";
    html += "<h3 style='margin:0;'>Last 15 Transmitted POCSAG</h3>";
    html += "<span id='pocsag-count' style='font-size:0.8em;color:var(--text-secondary);'></span>";
    html += "</div>";
    html += "<div style='overflow-x:auto;'>";
    html += "<table style='width:100%;border-collapse:collapse;font-size:0.85em;'>";
    html += "<thead><tr style='border-bottom:1px solid var(--border-color);'>";
    html += "<th style='text-align:left;padding:4px 6px;color:var(--text-secondary);white-space:nowrap;'>Time</th>";
    html += "<th style='text-align:left;padding:4px 6px;color:var(--text-secondary);'>RIC</th>";
    html += "<th style='text-align:left;padding:4px 6px;color:var(--text-secondary);'>Message</th>";
    html += "</tr></thead>";
    html += "<tbody id='pocsag-tx-body'><tr><td colspan='3' style='padding:8px 6px;color:var(--text-secondary);'>No messages yet</td></tr></tbody>";
    html += "</table></div></div>";
  }

  // Cards 5/6/7: row grid
  html += "<div style='display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:20px;'>";

  // Card 5: Virtual OLED Display
  html += "<div class='feature'>";
  html += "<h3 style='margin-top:0;margin-bottom:14px;'>OLED Display</h3>";
  html += "<div style='display:flex;justify-content:center;'>";
  html += "<div style='background:#1a1a2e;border-radius:8px;padding:12px 16px;border:2px solid #333;box-shadow:0 4px 15px rgba(0,0,0,0.5);'>";
  html += "<canvas id='oled-canvas' width='256' height='128' style='display:block;border-radius:4px;image-rendering:pixelated;'></canvas>";
  html += "</div>";
  html += "</div>";
  html += "<div style='display:flex;align-items:center;justify-content:center;gap:10px;margin-top:12px;'>";
  html += "<span style='font-size:0.85em;color:var(--text-secondary);'>Display power</span>";
  html += "<label style='position:relative;display:inline-block;width:44px;height:24px;'>";
  html += "<input type='checkbox' id='oled-power-toggle' style='opacity:0;width:0;height:0;' onchange='toggleOledPower(this.checked)'>";
  html += "<span id='oled-power-slider' style='position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#555;border-radius:24px;transition:0.3s;'>";
  html += "<span style='position:absolute;height:18px;width:18px;left:3px;bottom:3px;background:#fff;border-radius:50%;transition:0.3s;display:block;' id='oled-power-knob'></span></span></label>";
  html += "</div>";
  html += "<div style='display:flex;align-items:center;justify-content:center;gap:8px;margin-top:10px;'>";
  html += "<span style='font-size:0.85em;color:var(--text-secondary);'>Brightness</span>";
  html += "<button id='oled-br-dim'  onclick='setOledBrightness(\"dim\")'  style='padding:3px 10px;border-radius:6px;border:1px solid #555;cursor:pointer;font-size:0.8em;'>Dim</button>";
  html += "<button id='oled-br-mid'  onclick='setOledBrightness(\"mid\")'  style='padding:3px 10px;border-radius:6px;border:1px solid #555;cursor:pointer;font-size:0.8em;'>Mid</button>";
  html += "<button id='oled-br-full' onclick='setOledBrightness(\"full\")' style='padding:3px 10px;border-radius:6px;border:1px solid #555;cursor:pointer;font-size:0.8em;'>Full</button>";
  html += "</div>";
  html += "</div>";

  // Card 6: Mode Status
  html += "<div class='feature'>";
  html += "<h3 style='margin-top:0;margin-bottom:14px;'>Modes</h3>";
  html += "<div id='mode-status-list' style='display:flex;flex-direction:column;gap:8px;'>";
  html += "<span style='color:var(--text-secondary);font-size:0.85em;'>Loading...</span>";
  html += "</div>";
  html += "</div>";

  // Card 7: Service Status
  html += "<div class='feature'>";
  html += "<h3 style='margin-top:0;margin-bottom:14px;'>Services</h3>";
  html += "<div id='service-status-list' style='display:flex;flex-direction:column;gap:8px;'>";
  html += "<span style='color:var(--text-secondary);font-size:0.85em;'>Loading...</span>";
  html += "</div>";
  html += "</div>";

  html += "</div>"; // end cards 5/6/7 row

  html += "</div>"; // end stacked cards section

  } // end else (configured callsign)

  // Virtual OLED JavaScript
  html += "<script>";
  html += "var oledCanvas = document.getElementById('oled-canvas');";
  html += "var oledCtx = oledCanvas.getContext('2d');";
  html += "oledCtx.fillStyle = '#000';";
  html += "oledCtx.fillRect(0, 0, 256, 128);";
  html += "function renderOled(buf) {";
  html += "  var img = oledCtx.createImageData(128, 64);";
  html += "  for (var page = 0; page < 8; page++) {";
  html += "    for (var x = 0; x < 128; x++) {";
  html += "      var b = buf[page * 128 + x];";
  html += "      for (var bit = 0; bit < 8; bit++) {";
  html += "        var y = page * 8 + bit;";
  html += "        var on = (b >> bit) & 1;";
  html += "        var idx = (y * 128 + x) * 4;";
  html += "        img.data[idx] = on ? 0 : 0;";
  html += "        img.data[idx+1] = on ? 191 : 0;";
  html += "        img.data[idx+2] = on ? 255 : 0;";
  html += "        img.data[idx+3] = 255;";
  html += "      }";
  html += "    }";
  html += "  }";
  html += "  var tmp = document.createElement('canvas');";
  html += "  tmp.width = 128; tmp.height = 64;";
  html += "  tmp.getContext('2d').putImageData(img, 0, 0);";
  html += "  oledCtx.imageSmoothingEnabled = false;";
  html += "  oledCtx.drawImage(tmp, 0, 0, 256, 128);";
  html += "}";
  html += "function pollOled() {";
  html += "  fetch('/api/oled-framebuffer').then(function(r) { return r.arrayBuffer(); }).then(function(ab) {";
  html += "    renderOled(new Uint8Array(ab));";
  html += "  }).catch(function(){});";
  html += "}";
  html += "setInterval(pollOled, 500);";
  html += "pollOled();";
  html += "function setOledToggleState(on) {";
  html += "  var cb = document.getElementById('oled-power-toggle'); if(!cb) return;";
  html += "  cb.checked = on;";
  html += "  var sl = document.getElementById('oled-power-slider');";
  html += "  var kn = document.getElementById('oled-power-knob');";
  html += "  if(sl) sl.style.background = on ? '#28a745' : '#555';";
  html += "  if(kn) kn.style.transform = on ? 'translateX(20px)' : 'translateX(0)';";
  html += "}";
  html += "function toggleOledPower(on) {";
  html += "  fetch('/api/oled-power', {method:'POST'}).then(function(r){return r.json();}).then(function(d){ setOledToggleState(d.on); }).catch(function(){});";
  html += "}";
  html += "fetch('/api/oled-power').then(function(r){return r.json();}).then(function(d){ setOledToggleState(d.on); }).catch(function(){});";
  html += "function setOledBrightnessState(level) {";
  html += "  var btns = {dim:'oled-br-dim',mid:'oled-br-mid',full:'oled-br-full'};";
  html += "  Object.keys(btns).forEach(function(k) {";
  html += "    var b = document.getElementById(btns[k]); if(!b) return;";
  html += "    b.style.background = (k === level) ? '#28a745' : '';";
  html += "    b.style.color      = (k === level) ? '#fff'    : '';";
  html += "    b.style.borderColor= (k === level) ? '#28a745' : '#555';";
  html += "  });";
  html += "}";
  html += "function setOledBrightness(level) {";
  html += "  fetch('/api/oled-brightness?level='+level, {method:'POST'}).then(function(r){return r.json();}).then(function(d){ setOledBrightnessState(d.level); }).catch(function(){});";
  html += "}";
  html += "fetch('/api/oled-brightness').then(function(r){return r.json();}).then(function(d){ setOledBrightnessState(d.level); }).catch(function(){});";

  // DMR activity JS — only injected when at least one voice mode is enabled
  if (anyVoiceModeEnabled) {
  html += "var dmrHistory = [];";
  html += getCountryFlagJS();
  html += "var onairCallsign = '';";
  html += "var onairStart = 0;";
  html += "var titleBase = '" + mdnsHostname + "';";
  html += "var titleIdle = '" + userCallsign + " - " + mdnsHostname + "';";

  // Live On Air timer (ticks every second)
  html += "function fmtSec(s) { return Math.floor(s/60)+':'+(s%60<10?'0':'')+s%60; }";
  html += "setInterval(function() {";
  html += "  if (onairStart > 0) {";
  html += "    var elapsed = Math.floor((Date.now() - onairStart) / 1000);";
  html += "    var col = elapsed <= 120 ? '#28a745' : elapsed <= 150 ? '#fd7e14' : '#dc3545';";
  html += "    var txt = fmtSec(elapsed);";
  html += "    var timer = document.getElementById('onair-timer'); if(timer){timer.textContent=txt;timer.style.color=col;}";
  html += "    var liveDur = document.getElementById('live-dur'); if(liveDur){liveDur.textContent=txt;liveDur.style.color=col;}";
  html += "  }";
  html += "}, 1000);";

  // Render history table
  html += "function renderHistory() {";
  html += "  var minDur = parseInt(document.getElementById('dur-filter').value, 10);";
  html += "  var tbody = document.getElementById('call-history-body');";
  html += "  if (!tbody) return;";
  html += "  var filtered = dmrHistory.filter(function(h, i) {";
  html += "    var isActiveLive = (i === 0 && onairStart > 0 && h.duration === 0);";
  html += "    return isActiveLive || h.duration >= minDur;";
  html += "  });";
  html += "  if (filtered.length === 0) {";
  html += "    tbody.innerHTML = '<tr><td colspan=\"6\" style=\"padding:8px 6px;color:var(--text-secondary);\">'+(dmrHistory.length===0?'No calls yet':'No calls match filter')+'</td></tr>';";
  html += "    return;";
  html += "  }";
  html += "  var rows = '';";
  html += "  for (var i = 0; i < filtered.length; i++) {";
  html += "    var h = filtered[i];";
  html += "    var isLive = (i === 0 && onairStart > 0 && onairCallsign && h.callsign === onairCallsign && h.duration === 0);";
  html += "    var bg = isLive ? 'background:rgba(220,53,69,0.07);' : (i%2===0 ? '' : 'background:rgba(128,128,128,0.05);');";
  html += "    var durHtml, durStyle;";
  html += "    if (isLive) {";
  html += "      var el0 = Math.floor((Date.now() - onairStart) / 1000);";
  html += "      var lc = el0 <= 120 ? '#28a745' : el0 <= 150 ? '#fd7e14' : '#dc3545';";
  html += "      durHtml = '<span id=\"live-dur\" style=\"color:'+lc+';font-weight:bold;\">'+fmtSec(el0)+'</span>';";
  html += "      durStyle = 'padding:4px 6px;white-space:nowrap;';";
  html += "    } else {";
  html += "      durHtml = (h.duration > 0) ? fmtSec(h.duration) : '-';";
  html += "      durStyle = 'padding:4px 6px;color:var(--text-secondary);white-space:nowrap;';";
  html += "    }";
  html += "    rows += '<tr style=\"' + bg + '\">';";
  html += "    rows += '<td style=\"padding:4px 6px;color:var(--text-secondary);white-space:nowrap;\">' + (h.time||'') + (isLive ? ' <span style=\"color:#dc3545;font-size:0.7em;\">&#9679;</span>' : '') + '</td>';";
  html += "    rows += '<td style=\"'+durStyle+'\">' + durHtml + '</td>';";
  html += "    var flag = countryFlag(h.country||'');";
  html += "    var flagHtml = flag ? '<span title=\"'+(h.country||'')+'\" style=\"cursor:default;\">'+flag+'</span> ' : '';";
  html += "    rows += '<td style=\"padding:4px 6px;font-weight:bold;\">' + flagHtml + (h.callsign||'') + '</td>';";
  html += "    rows += '<td style=\"padding:4px 6px;\">' + (h.name||'') + '</td>';";
  html += "    rows += '<td style=\"padding:4px 6px;\">' + (h.city||'') + '</td>';";
  html += "    rows += '<td style=\"padding:4px 6px;\">' + (h.country||'') + '</td>';";
  html += "    rows += '</tr>';";
  html += "  }";
  html += "  tbody.innerHTML = rows;";
  html += "}";
  html += "var durFilter = document.getElementById('dur-filter');";
  html += "var savedDur = localStorage.getItem('durFilter');";
  html += "if (savedDur !== null) { durFilter.value = savedDur; }";
  html += "durFilter.addEventListener('change', function() {";
  html += "  localStorage.setItem('durFilter', durFilter.value);";
  html += "  renderHistory();";
  html += "});";

  // DMR activity poll
  html += "function pollDmrActivity() {";
  html += "  fetch('/api/dmr-activity').then(function(r){return r.json();}).then(function(d){";
  // On Air badge
  html += "    var badge = document.getElementById('onair-badge');";
  html += "    if (badge) {";
  html += "      if (d.active) { badge.textContent='ON AIR'; badge.style.background='#dc3545'; badge.style.fontSize='1.1em'; badge.style.padding='6px 20px'; }";
  html += "      else          { badge.textContent='IDLE';   badge.style.background='#555';    badge.style.fontSize='1.1em'; badge.style.padding='6px 20px'; }";
  html += "    }";
  html += "    document.title = (d.active && d.current.callsign) ? '>>' + d.current.callsign + '<< - ' + titleBase : titleIdle;";
  // On Air timer: reset on new callsign, clear on IDLE
  html += "    var c = d.current;";
  html += "    if (d.active && c.callsign && c.callsign !== onairCallsign) {";
  html += "      onairCallsign = c.callsign;";
  html += "      onairStart = Date.now() - (d.elapsed || 0) * 1000;";
  html += "      var initSec = d.elapsed || 0;";
  html += "      var col = initSec <= 120 ? '#28a745' : initSec <= 150 ? '#fd7e14' : '#dc3545';";
  html += "      var timer=document.getElementById('onair-timer'); if(timer){timer.textContent=fmtSec(initSec);timer.style.color=col;}";
  html += "    } else if (!d.active && onairCallsign !== '') {";
  html += "      onairCallsign = '';";
  html += "      onairStart = 0;";
  html += "      var timer=document.getElementById('onair-timer'); if(timer)timer.textContent='';";
  html += "    }";
  // Current call info — card glow + info line
  html += "    var loc = [c.city, c.country].filter(Boolean).join(', ');";
  html += "    var card = document.getElementById('onair-card');";
  html += "    if(card) {";
  html += "      card.style.borderColor = d.active ? '#dc3545' : '';";
  html += "      card.style.boxShadow   = d.active ? '0 0 0 1px #dc3545,0 4px 24px rgba(220,53,69,0.25)' : '';";
  html += "    }";
  html += "    var info = document.getElementById('onair-info');";
  html += "    if(info) {";
  html += "      if(d.active && c.callsign) {";
  html += "        var flag = countryFlag(c.country||'');";
  html += "        var ih = '';";
  html += "        if(flag) ih += '<span style=\"font-size:2.2em;line-height:1;\" title=\"'+(c.country||'')+'\">'+flag+'</span>';";
  html += "        ih += '<span style=\"font-size:1.6em;font-weight:bold;letter-spacing:2px;color:var(--text-primary);\">'+c.callsign+'</span>';";
  html += "        if(c.name) ih += '<span style=\"color:var(--text-secondary);padding:0 4px;\">·</span><span style=\"font-size:1em;\">'+c.name+'</span>';";
  html += "        if(loc)    ih += '<span style=\"color:var(--text-secondary);padding:0 4px;\">·</span><span style=\"font-size:0.9em;color:var(--text-secondary);\">'+loc+'</span>';";
  html += "        info.innerHTML = ih;";
  html += "      } else { info.innerHTML = ''; }";
  html += "    }";
  // Update history and re-render
  html += "    dmrHistory = d.history || [];";
  html += "    renderHistory();";
  html += "  }).catch(function(){});";
  html += "}";
  html += "setInterval(pollDmrActivity, 2000);";
  html += "pollDmrActivity();";
  } // end anyVoiceModeEnabled JS block

  // POCSAG TX history polling
  html += "function pollPocsagTx() {";
  html += "  fetch('/api/pocsag-tx-history').then(function(r){return r.json();}).then(function(d){";
  html += "    var cnt = document.getElementById('pocsag-count'); if(cnt) cnt.textContent = d.count+' / 15';";
  html += "    var tbody = document.getElementById('pocsag-tx-body'); if(!tbody) return;";
  html += "    if (!d.items || d.items.length===0) { tbody.innerHTML='<tr><td colspan=\"3\" style=\"padding:8px 6px;color:var(--text-secondary);\">No messages yet</td></tr>'; return; }";
  html += "    var rows='';";
  html += "    for(var i=0;i<d.items.length;i++){";
  html += "      var p=d.items[i]; var bg=(i%2===0)?'':'background:rgba(128,128,128,0.05);';";
  html += "      rows+='<tr style=\"'+bg+'\"><td style=\"padding:4px 6px;color:var(--text-secondary);white-space:nowrap;\">'+(p.time||'')+'</td>';";
  html += "      rows+='<td style=\"padding:4px 6px;font-weight:bold;white-space:nowrap;\">'+(p.ric||'')+'</td>';";
  html += "      rows+='<td style=\"padding:4px 6px;word-break:break-word;\">'+(p.msg||'')+'</td></tr>';";
  html += "    }";
  html += "    tbody.innerHTML=rows;";
  html += "  }).catch(function(){});";
  html += "}";
  html += "setInterval(pollPocsagTx, 5000);";
  html += "pollPocsagTx();";

  // POCSAG queue polling
  html += "function pollPocsagQueue() {";
  html += "  fetch('/api/pocsag-queue').then(function(r){return r.json();}).then(function(d){";
  html += "    var st=document.getElementById('queue-status'); if(st) st.textContent=d.count+' / '+d.capacity;";
  html += "    var tbody=document.getElementById('pocsag-queue-body'); if(!tbody) return;";
  html += "    if(!d.items||d.items.length===0){tbody.innerHTML='<tr><td colspan=\"2\" style=\"padding:8px 6px;color:var(--text-secondary);\">Queue empty</td></tr>';return;}";
  html += "    var rows='';";
  html += "    for(var i=0;i<d.items.length;i++){";
  html += "      var q=d.items[i]; var bg=(i%2===0)?'':'background:rgba(128,128,128,0.05);';";
  html += "      rows+='<tr style=\"'+bg+'\"><td style=\"padding:4px 6px;font-weight:bold;white-space:nowrap;\">'+(q.ric||'')+'</td>';";
  html += "      rows+='<td style=\"padding:4px 6px;word-break:break-word;\">'+(q.msg||'')+'</td></tr>';";
  html += "    }";
  html += "    tbody.innerHTML=rows;";
  html += "  }).catch(function(){});";
  html += "}";
  html += "setInterval(pollPocsagQueue, 3000);";
  html += "pollPocsagQueue();";

  // DAPNET received message history polling
  html += "function pollDapnetHistory() {";
  html += "  var tbody=document.getElementById('dapnet-hist-body'); if(!tbody) return;";
  html += "  fetch('/api/dapnet-history').then(function(r){return r.json();}).then(function(d){";
  html += "    var cnt=document.getElementById('dapnet-hist-count'); if(cnt) cnt.textContent=d.count+' / 15';";
  html += "    if(!d.items||d.items.length===0){tbody.innerHTML='<tr><td colspan=\"3\" style=\"padding:8px 6px;color:var(--text-secondary);\">No messages yet</td></tr>';return;}";
  html += "    var rows='';";
  html += "    for(var i=d.items.length-1;i>=0;i--){";
  html += "      var m=d.items[i]; var bg=(i%2===0)?'':'background:rgba(128,128,128,0.05);';";
  html += "      rows+='<tr style=\"'+bg+'\">';";
  html += "      rows+='<td style=\"padding:4px 6px;white-space:nowrap;\">'+m.time+'</td>';";
  html += "      rows+='<td style=\"padding:4px 6px;font-weight:bold;\">'+m.ric+'</td>';";
  html += "      rows+='<td style=\"padding:4px 6px;word-break:break-word;\">'+m.msg+'</td>';";
  html += "      rows+='</tr>';";
  html += "    }";
  html += "    tbody.innerHTML=rows;";
  html += "  }).catch(function(){});";
  html += "}";
  html += "setInterval(pollDapnetHistory, 10000);";
  html += "pollDapnetHistory();";

  // Service status polling
  html += "function renderStatusList(listId, items) {";
  html += "  var list = document.getElementById(listId); if(!list) return;";
  html += "  items.sort(function(a,b){";
  html += "    function pri(m){ if(!m.enabled)return 3; if('connected'in m&&!m.connected)return 0; if('connected'in m&&m.connected)return 1; return 2; }";
  html += "    var d=pri(a)-pri(b); return d!==0?d:a.label.localeCompare(b.label);";
  html += "  });";
  html += "  var h='';";
  html += "  for(var i=0;i<items.length;i++){";
  html += "    var m=items[i]; var col,txt;";
  html += "    if(!m.enabled){ col='#555'; txt='DISABLED'; }";
  html += "    else if('connected'in m&&!m.connected){ col='#fd7e14'; txt='NOT CONNECTED'; }";
  html += "    else if('connected'in m&&m.connected&&m.softap){ col='#17a2b8'; txt='SOFTAP'; }";
  html += "    else if('connected'in m&&m.connected){ col='#28a745'; txt='CONNECTED'; }";
  html += "    else{ col='#28a745'; txt='ENABLED'; }";
  html += "    h+='<div style=\"display:flex;align-items:center;justify-content:space-between;\">';";
  html += "    h+='<span style=\"font-weight:bold;font-size:0.9em;\">'+m.label+'</span>';";
  html += "    h+='<span style=\"font-size:0.75em;font-weight:bold;padding:2px 10px;border-radius:12px;background:'+col+';color:#fff;\">'+txt+'</span>';";
  html += "    h+='</div>';";
  html += "  }";
  html += "  list.innerHTML=h;";
  html += "}";
  html += "function pollServiceStatus() {";
  html += "  fetch('/api/service-status').then(function(r){return r.json();}).then(function(d){";
  html += "    renderStatusList('service-status-list', d);";
  html += "  }).catch(function(){});";
  html += "}";
  html += "setInterval(pollServiceStatus, 5000);";
  html += "pollServiceStatus();";

  // Mode status polling
  html += "function pollModeStatus() {";
  html += "  fetch('/api/mode-status').then(function(r){return r.json();}).then(function(d){";
  html += "    renderStatusList('mode-status-list', d);";
  html += "  }).catch(function(){});";
  html += "}";
  html += "setInterval(pollModeStatus, 5000);";
  html += "pollModeStatus();";

  html += "</script>";

  html += "</div>";
  html += getFooter();
  html += "</body></html>";

  return html;
}

#endif // WEB_MAIN_H
