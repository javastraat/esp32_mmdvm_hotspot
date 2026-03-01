#ifndef WEB_MODE_DMR_H
#define WEB_MODE_DMR_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"
#include "web/include/bm_servers.h"

// Externs for settings variables (should match your main project)
extern String userCallsign;
extern uint32_t userDmrId;
extern String dmrServer;
extern String dmrPassword;
extern uint8_t userDmrSsid;
extern uint32_t dmrRxFreq;
extern uint32_t dmrTxFreq;
extern uint8_t dmrColorCode;
extern uint8_t dmrRfPower;
extern String hotspotCallsign;
extern String hotspotSuffix;
extern String hotspotLatitude;
extern String hotspotLongitude;
extern int hotspotHeight;
extern String hotspotLocation;
extern String hotspotDescription;
extern String hotspotUrl;
extern bool modeDmrEnabled;

String getModeDmrPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>DMR & Hotspot Configuration</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("mode-dmr");
  html += "<div class='container'>";
  html += "<h1>DMR & Hotspot Configuration</h1>";
  html += "<div class='admin-grid'>";

  // Card 1: DMR Mode Status & Enable
  html += "<div class='card'>";
  html += "<h3>DMR Status</h3>";
  html += "<div class='metric'><span class='metric-label'>Status:</span><span class='metric-value'>" + String(modeDmrEnabled ? "Enabled" : "Disabled") + "</span></div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enable DMR:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='mode-dmr'" + String(modeDmrEnabled ? " checked" : "") + ">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveDmrMode()'>Save & Reboot</button>";
  html += "<button class='btn btn-danger' onclick='resetDmrMode()'>Reset to Defaults</button>";
  html += "</div>";
  html += "</div>";

  // Card 2: Station Information
  html += "<div class='card'>";
  html += "<h3>Station Information</h3>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Callsign:</span>";
  html += "<input type='text' id='callsign' value='" + userCallsign + "' maxlength='10' style='width: 120px; padding-right: 8px; text-transform: uppercase;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>DMR ID:</span>";
  html += "<input type='text' id='dmrid' value='" + String(userDmrId) + "' maxlength='7' inputmode='numeric' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>SSID (0-99):</span>";
  html += "<input type='text' id='ssid' value='" + String(userDmrSsid) + "' maxlength='2' inputmode='numeric' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "Your amateur radio callsign, DMR ID, and SSID.";
  html += "</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveStationInfo()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetStationInfo()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";


  // Card 2: BrandMeister Network
  html += "<div class='card'>";
  html += "<h3>BrandMeister Network</h3>";
  html += "<form id='form-network' onsubmit='return saveDmrNetwork(event)'>";
  // BrandMeister server pulldown and free input (server list from bm_servers.h)
  bool isKnownServer = false;
  for (int i = 0; i < bmServerCount; i++) {
    if (dmrServer == String(bmServers[i].address)) { isKnownServer = true; break; }
  }
  html += "<div class='metric'><span class='metric-label'>Server:</span>";
  html += "<select id='serverSelect' style='width: 60%; margin-right: 8px;' onchange='updateServerField()'>";
  html += "<option value='custom'" + String(!isKnownServer ? " selected" : "") + ">Custom Server (enter below)</option>";
  for (int i = 0; i < bmServerCount; i++) {
    html += "<option value='";
    html += bmServers[i].address;
    html += "'";
    if (dmrServer == String(bmServers[i].address)) html += " selected";
    html += ">";
    html += bmServers[i].name;
    html += "</option>";
  }
  html += "</select>";
  html += "<input type='text' id='serverInput' name='dmr_server' placeholder='IP or FQDN' value='" + dmrServer + "' required style='width: 38%; display: inline-block;'>";
  html += "</div>";
  html += "<div class='metric'><span class='metric-label'>Password:</span><input type='password' name='dmr_password' value='" + dmrPassword + "' required></div>";
  html += "<div class='metric'><span class='metric-label'>RX Frequency (Hz):</span><input type='text' name='dmr_rx_freq' value='" + String(dmrRxFreq) + "' required maxlength='10' pattern='\\d{1,10}' title='Enter up to 10 digits'></div>";
  html += "<div class='metric'><span class='metric-label'>TX Frequency (Hz):</span><input type='text' name='dmr_tx_freq' value='" + String(dmrTxFreq) + "' required maxlength='10' pattern='\\d{1,10}' title='Enter up to 10 digits'></div>";
  html += "<div class='metric'><span class='metric-label'>Color Code (1-15):</span><input type='text' name='dmr_color_code' value='" + String(dmrColorCode) + "' required maxlength='2' pattern='1[0-5]|[1-9]' title='Enter 1-15'></div>";
  html += "<div class='metric'><span class='metric-label'>RF Power (0-255):</span><input type='text' name='dmr_rf_power' value='" + String(dmrRfPower) + "' required maxlength='3' pattern='255|[0-9]{1,2}' title='Enter 0-255'></div>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button class='btn btn-success' type='submit'>Save</button>";
  html += "<button class='btn btn-danger' type='button' onclick='resetDmrNetwork()'>Reset to Default</button>";
  html += "</div>";
  html += "</form>";
  html += "<div id='network-status' style='margin-top: 10px; padding: 10px; display: none;'></div>";
  html += "</div>";

  // Card 3: Hotspot Location & Info
  html += "<div class='card'>";
  html += "<h3>Hotspot Location & Info</h3>";
  html += "<form id='form-hotspot' onsubmit='return saveHotspotInfo(event)'>";
  html += "<div class='metric'><span class='metric-label'>Hotspot Callsign:</span><input type='text' name='hs_callsign' value='" + hotspotCallsign + "'></div>";
  html += "<div class='metric'><span class='metric-label'>Suffix:</span><input type='text' name='hs_suffix' value='" + hotspotSuffix + "'></div>";
  html += "<div class='metric'><span class='metric-label'>Latitude:</span><input type='text' name='hs_latitude' value='" + hotspotLatitude + "' placeholder='0.0'></div>";
  html += "<div class='metric'><span class='metric-label'>Longitude:</span><input type='text' name='hs_longitude' value='" + hotspotLongitude + "' placeholder='0.0'></div>";
  html += "<div class='metric'><span class='metric-label'>Height (meters):</span><input type='number' name='hs_height' value='" + String(hotspotHeight) + "'></div>";
  html += "<div class='metric'><span class='metric-label'>Location:</span><input type='text' name='hs_location' value='" + hotspotLocation + "'></div>";
  html += "<div class='metric'><span class='metric-label'>Description:</span><input type='text' name='hs_desc' value='" + hotspotDescription + "'></div>";
  html += "<div class='metric'><span class='metric-label'>URL:</span><input type='text' name='hs_url' value='" + hotspotUrl + "'></div>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button class='btn btn-success' type='submit'>Save</button>";
  html += "<button class='btn btn-danger' type='button' onclick='resetHotspotInfo()'>Reset to Default</button>";
  html += "</div>";
  html += "</form>";
  html += "<div id='hotspot-status' style='margin-top: 10px; padding: 10px; display: none;'></div>";
  html += "</div>";

  // Card 4: Mode Enable
  /*
  html += "<div class='card'>";
  html += "<h3>Mode Enable</h3>";
  html += "<form id='form-mode' action='/save_mode_enable' method='POST' onsubmit='return saveModeEnable(event)'>";
  html += "<div class='metric'><span class='metric-label'>Enable DMR Mode:</span><input type='checkbox' name='mode_dmr_enabled' value='1'" + String(modeDmrEnabled ? " checked" : "") + "></div>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button class='btn btn-success' type='submit'>Save</button>";
  html += "<button class='btn btn-danger' type='button' onclick='resetCard(\"mode\")'>Reset to Default</button>";
  html += "</div>";
  html += "</form>";
  html += "<div id='mode-status' style='margin-top: 10px; padding: 10px; display: none;'></div>";
  html += "</div>";
  */

  html += "</div>";

  // Save All & Reboot button
  html += "<div class='action-buttons-vertical' style='margin-top:20px;'>";
  html += "<button class='btn btn-success' onclick='saveAllSettings()'>Save All & Reboot</button>";
  html += "</div>";
  html += "<div class='info' style='margin-top:20px'>";
  html += "<strong>Note:</strong> After changing settings, click Save All & Reboot for all changes to take effect.";
  html += "</div>";

  // ===============================
  // Functions and AJAX handlers
  // ===============================
  //
  // JavaScript functions
  html += "<script>\n";
  // Modal helpers
  html += "function showModal(contentFn) {\n";
  html += "  var overlay = document.createElement('div');\n";
  html += "  overlay.className = 'modal-overlay';\n";
  html += "  var box = document.createElement('div');\n";
  html += "  box.className = 'modal-box';\n";
  html += "  contentFn(box, function() { document.body.removeChild(overlay); });\n";
  html += "  overlay.appendChild(box);\n";
  html += "  overlay.addEventListener('click', function(e) { if (e.target === overlay) document.body.removeChild(overlay); });\n";
  html += "  document.body.appendChild(overlay);\n";
  html += "}\n";
  html += "function showAlert(msg, onOk) {\n";
  html += "  showModal(function(box, close) {\n";
  html += "    box.innerHTML = '<h4>' + msg + '</h4>';\n";
  html += "    var btns = document.createElement('div');\n";
  html += "    btns.className = 'modal-buttons';\n";
  html += "    var ok = document.createElement('button');\n";
  html += "    ok.textContent = 'OK';\n";
  html += "    ok.className = 'btn btn-primary';\n";
  html += "    ok.onclick = function() { close(); if (onOk) onOk(); };\n";
  html += "    btns.appendChild(ok);\n";
  html += "    box.appendChild(btns);\n";
  html += "  });\n";
  html += "}\n";
  html += "function showConfirm(msg, onYes) {\n";
  html += "  showModal(function(box, close) {\n";
  html += "    box.innerHTML = '<h4>' + msg + '</h4>';\n";
  html += "    var btns = document.createElement('div');\n";
  html += "    btns.className = 'modal-buttons';\n";
  html += "    var yes = document.createElement('button');\n";
  html += "    yes.textContent = 'Yes';\n";
  html += "    yes.className = 'btn btn-success';\n";
  html += "    yes.onclick = function() { close(); onYes(); };\n";
  html += "    var no = document.createElement('button');\n";
  html += "    no.textContent = 'Cancel';\n";
  html += "    no.className = 'btn btn-danger';\n";
  html += "    no.onclick = close;\n";
  html += "    btns.appendChild(yes);\n";
  html += "    btns.appendChild(no);\n";
  html += "    box.appendChild(btns);\n";
  html += "  });\n";
  html += "}\n";
  html += "function saveAndReboot(msg) {\n";
  html += "  showAlert(msg + '<br><br>The device will now reboot.', function() {\n";
  html += "    fetch('/api/reboot', {method: 'POST'});\n";
  html += "    document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';\n";
  html += "    setTimeout(function() { location.reload(); }, 10000);\n";
  html += "  });\n";
  html += "}\n";
  //
  //
  // Station Info Card functions
  html += "function saveStationInfo() {\n";
  html += "  var callsign = document.getElementById('callsign').value.toUpperCase();\n";
  html += "  var dmrid = document.getElementById('dmrid').value;\n";
  html += "  var ssid = document.getElementById('ssid').value;\n";
  html += "  if (callsign.length < 3) { showAlert('Callsign must be at least 3 characters'); return; }\n";
  html += "  if (!dmrid || !/^[0-9]{1,7}$/.test(dmrid)) { showAlert('DMR ID must be 1-7 digits'); return; }\n";
  html += "  if (ssid === '' || !/^[0-9]{1,2}$/.test(ssid) || ssid < 0 || ssid > 99) { showAlert('SSID must be 0-99'); return; }\n";
  html += "  showConfirm('Save station info: ' + callsign + '-' + ssid + ' / ' + dmrid + '?', function() {\n";
  html += "    fetch('/api/save-station', {\n";
  html += "      method: 'POST',\n";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "      body: 'callsign=' + encodeURIComponent(callsign) + '&dmrid=' + dmrid + '&ssid=' + ssid\n";
  html += "    }).then(r => r.text()).then(msg => { saveAndReboot(msg); });\n";
  html += "  });\n";
  html += "}\n";
  html += "function resetStationInfo() {\n";
  html += "  showConfirm('Reset station info to defaults?<br><br>Callsign: " + String(DMR_CALLSIGN) + "-" + String(DMR_SSID) + "<br>DMR ID: " + String(DMR_ID) + "', function() {\n";
  html += "    fetch('/api/reset-station', {method: 'POST'}).then(r => r.text()).then(msg => { saveAndReboot(msg); });\n";
  html += "  });\n";
  html += "}\n";
  //
  // Server dropdown helper
  html += "function updateServerField() {\n";
  html += "  var sel = document.getElementById('serverSelect');\n";
  html += "  var inp = document.getElementById('serverInput');\n";
  html += "  if(sel.value === 'custom') { inp.value = ''; inp.focus(); } else { inp.value = sel.value; }\n";
  html += "}\n";
  //
  // Card 2: BrandMeister Network save/reset
  html += "function saveDmrNetwork(e) {\n";
  html += "  e.preventDefault();\n";
  html += "  var server = document.getElementById('serverInput').value;\n";
  html += "  var password = document.querySelector('[name=dmr_password]').value;\n";
  html += "  var rxfreq = document.querySelector('[name=dmr_rx_freq]').value;\n";
  html += "  var txfreq = document.querySelector('[name=dmr_tx_freq]').value;\n";
  html += "  var colorcode = document.querySelector('[name=dmr_color_code]').value;\n";
  html += "  var rfpower = document.querySelector('[name=dmr_rf_power]').value;\n";
  html += "  if (!server) { showAlert('Server address is required'); return false; }\n";
  html += "  if (!password) { showAlert('Password is required'); return false; }\n";
  html += "  if (!rxfreq || !/^\\d{1,10}$/.test(rxfreq)) { showAlert('RX Frequency must be numeric'); return false; }\n";
  html += "  if (!txfreq || !/^\\d{1,10}$/.test(txfreq)) { showAlert('TX Frequency must be numeric'); return false; }\n";
  html += "  if (!colorcode || colorcode < 1 || colorcode > 15) { showAlert('Color Code must be 1-15'); return false; }\n";
  html += "  showConfirm('Save BrandMeister network settings?<br><br>Server: ' + server, function() {\n";
  html += "    fetch('/api/save-dmr-network', {\n";
  html += "      method: 'POST',\n";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "      body: 'server=' + encodeURIComponent(server) + '&password=' + encodeURIComponent(password) + '&rxfreq=' + rxfreq + '&txfreq=' + txfreq + '&colorcode=' + colorcode + '&rfpower=' + rfpower\n";
  html += "    }).then(r => r.text()).then(msg => { saveAndReboot(msg); });\n";
  html += "  });\n";
  html += "  return false;\n";
  html += "}\n";
  html += "function resetDmrNetwork() {\n";
  html += "  showConfirm('Reset BrandMeister network settings to defaults?', function() {\n";
  html += "    fetch('/api/reset-dmr-network', {method: 'POST'}).then(r => r.text()).then(msg => { saveAndReboot(msg); });\n";
  html += "  });\n";
  html += "}\n";
  //
  // Card 3: Hotspot Info save/reset
  html += "function saveHotspotInfo(e) {\n";
  html += "  e.preventDefault();\n";
  html += "  var hs_callsign = document.querySelector('[name=hs_callsign]').value;\n";
  html += "  var hs_suffix = document.querySelector('[name=hs_suffix]').value;\n";
  html += "  var hs_latitude = document.querySelector('[name=hs_latitude]').value;\n";
  html += "  var hs_longitude = document.querySelector('[name=hs_longitude]').value;\n";
  html += "  var hs_height = document.querySelector('[name=hs_height]').value;\n";
  html += "  var hs_location = document.querySelector('[name=hs_location]').value;\n";
  html += "  var hs_desc = document.querySelector('[name=hs_desc]').value;\n";
  html += "  var hs_url = document.querySelector('[name=hs_url]').value;\n";
  html += "  showConfirm('Save hotspot info?<br><br>Callsign: ' + hs_callsign + '-' + hs_suffix, function() {\n";
  html += "    fetch('/api/save-hotspot', {\n";
  html += "      method: 'POST',\n";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "      body: 'hs_callsign=' + encodeURIComponent(hs_callsign) + '&hs_suffix=' + encodeURIComponent(hs_suffix) + '&hs_latitude=' + hs_latitude + '&hs_longitude=' + hs_longitude + '&hs_height=' + hs_height + '&hs_location=' + encodeURIComponent(hs_location) + '&hs_desc=' + encodeURIComponent(hs_desc) + '&hs_url=' + encodeURIComponent(hs_url)\n";
  html += "    }).then(r => r.text()).then(msg => { saveAndReboot(msg); });\n";
  html += "  });\n";
  html += "  return false;\n";
  html += "}\n";
  html += "function resetHotspotInfo() {\n";
  html += "  showConfirm('Reset hotspot info to defaults?', function() {\n";
  html += "    fetch('/api/reset-hotspot', {method: 'POST'}).then(r => r.text()).then(msg => { saveAndReboot(msg); });\n";
  html += "  });\n";
  html += "}\n";
  //
  // Card 1: DMR mode toggle
  html += "function saveDmrMode() {\n";
  html += "  var enable = document.getElementById('mode-dmr').checked;\n";
  html += "  showConfirm('Save DMR mode and reboot?', function() {\n";
  html += "    fetch('/api/mode-toggle', {\n";
  html += "      method: 'POST',\n";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "      body: 'mode=dmr&enable=' + enable\n";
  html += "    }).then(r => r.text()).then(msg => { saveAndReboot(msg); })\n";
  html += "      .catch(function() { showAlert('Failed to save settings.'); });\n";
  html += "  });\n";
  html += "}\n";
  html += "function resetDmrMode() {\n";
  html += "  showConfirm('Disable DMR mode and reboot?', function() {\n";
  html += "    fetch('/api/mode-toggle', {\n";
  html += "      method: 'POST',\n";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "      body: 'mode=dmr&enable=false'\n";
  html += "    }).then(r => r.text()).then(msg => { saveAndReboot(msg); })\n";
  html += "      .catch(function() { showAlert('Failed to reset settings.'); });\n";
  html += "  });\n";
  html += "}\n";

  // Save All & Reboot
  html += "function saveAllSettings() {\n";
  html += "  showConfirm('Save all settings and reboot?', function() {\n";
  html += "  var post = function(url, body) {\n";
  html += "    return fetch(url, { method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: body });\n";
  html += "  };\n";
  // Card 1: Station Info
  html += "  var callsign = document.getElementById('callsign').value.toUpperCase();\n";
  html += "  var dmrid = document.getElementById('dmrid').value;\n";
  html += "  var ssid = document.getElementById('ssid').value;\n";
  // Card 2: Network
  html += "  var server = document.getElementById('serverInput').value;\n";
  html += "  var password = document.querySelector('[name=dmr_password]').value;\n";
  html += "  var rxfreq = document.querySelector('[name=dmr_rx_freq]').value;\n";
  html += "  var txfreq = document.querySelector('[name=dmr_tx_freq]').value;\n";
  html += "  var colorcode = document.querySelector('[name=dmr_color_code]').value;\n";
  html += "  var rfpower = document.querySelector('[name=dmr_rf_power]').value;\n";
  // Card 3: Hotspot
  html += "  var hs_callsign = document.querySelector('[name=hs_callsign]').value;\n";
  html += "  var hs_suffix = document.querySelector('[name=hs_suffix]').value;\n";
  html += "  var hs_latitude = document.querySelector('[name=hs_latitude]').value;\n";
  html += "  var hs_longitude = document.querySelector('[name=hs_longitude]').value;\n";
  html += "  var hs_height = document.querySelector('[name=hs_height]').value;\n";
  html += "  var hs_location = document.querySelector('[name=hs_location]').value;\n";
  html += "  var hs_desc = document.querySelector('[name=hs_desc]').value;\n";
  html += "  var hs_url = document.querySelector('[name=hs_url]').value;\n";
  // Send all sequentially
  html += "  post('/api/save-station', 'callsign=' + encodeURIComponent(callsign) + '&dmrid=' + dmrid + '&ssid=' + ssid)\n";
  html += "  .then(function() { return post('/api/save-dmr-network', 'server=' + encodeURIComponent(server) + '&password=' + encodeURIComponent(password) + '&rxfreq=' + rxfreq + '&txfreq=' + txfreq + '&colorcode=' + colorcode + '&rfpower=' + rfpower); })\n";
  html += "  .then(function() { return post('/api/save-hotspot', 'hs_callsign=' + encodeURIComponent(hs_callsign) + '&hs_suffix=' + encodeURIComponent(hs_suffix) + '&hs_latitude=' + hs_latitude + '&hs_longitude=' + hs_longitude + '&hs_height=' + hs_height + '&hs_location=' + encodeURIComponent(hs_location) + '&hs_desc=' + encodeURIComponent(hs_desc) + '&hs_url=' + encodeURIComponent(hs_url)); })\n";
  html += "  .then(function() {\n";
  html += "    showAlert('All settings saved.<br><br>The device will now reboot.');\n";
  html += "    fetch('/api/reboot', {method: 'POST'});\n";
  html += "    document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';\n";
  html += "    setTimeout(function() { location.reload(); }, 10000);\n";
  html += "  }).catch(function(err) {\n";
  html += "    showAlert('Error saving settings: ' + err);\n";
  html += "  });\n";
  html += "  });\n";
  html += "}\n";



  html += "</script>\n";

  html += "</div>";
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_MODE_DMR_H
