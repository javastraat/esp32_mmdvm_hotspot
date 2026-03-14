/*
 * Hotspot Configuration Page
 * Station identity (callsign, DMR ID, SSID) and hotspot location/info.
 * These settings are shared across all modes.
 * DMR network settings → /mode-dmr
 */

#ifndef WEB_SYSTEM_HOTSPOT_H
#define WEB_SYSTEM_HOTSPOT_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"
#include "include/config.h"

extern String userCallsign;
extern uint32_t userDmrId;
extern uint8_t userDmrSsid;
extern String hotspotCallsign;
extern String hotspotSuffix;
extern String hotspotLatitude;
extern String hotspotLongitude;
extern int hotspotHeight;
extern String hotspotLocation;
extern String hotspotDescription;
extern String hotspotUrl;

String getSystemHotspotPageHTML()
{
  String html;
  html.reserve(38000);
  html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Hotspot Configuration</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("system-hotspot");
  html += "<div class='container'>";
  html += "<h1>Hotspot Configuration</h1>";
  html += "<p>Station identity and location settings shared across all modes.</p>";
  html += "<div class='admin-grid'>";

  // Card 1: Station Information
  html += "<div class='card'>";
  html += "<h3>Station Information</h3>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Callsign:</span>";
  html += "<input type='text' id='callsign' value='" + userCallsign + "' maxlength='10' style='width:120px;padding-right:8px;text-transform:uppercase;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>DMR ID:</span>";
  html += "<input type='text' id='dmrid' value='" + String(userDmrId) + "' maxlength='7' inputmode='numeric' style='width:120px;padding-right:8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>SSID (0-99):</span>";
  html += "<input type='text' id='ssid' value='" + String(userDmrSsid) + "' maxlength='2' inputmode='numeric' style='width:120px;padding-right:8px;'>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>Your amateur radio callsign, DMR ID, and SSID.</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveStationInfo()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetStationInfo()'>Reset to Default</button>";
  html += "</div>";
  html += "<div id='station-result' style='margin-top:8px;font-size:0.9em;'></div>";
  html += "</div>";

  // Card 2: Hotspot Location & Info
  html += "<div class='card'>";
  html += "<h3>Hotspot Location &amp; Info</h3>";
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
  html += "<div id='hotspot-status' style='margin-top:10px;padding:10px;display:none;'></div>";
  html += "</div>";

  html += "</div>"; // close admin-grid

  // Save All & Reboot button
  html += "<div class='action-buttons-vertical' style='margin-top:20px;'>";
  html += "<button class='btn btn-success' onclick='saveAllHotspotSettings()'>Save All &amp; Reboot</button>";
  html += "</div>";
  html += "<div class='info' style='margin-top:20px'>";
  html += "<strong>Note:</strong> After changing settings, click Save All &amp; Reboot for all changes to take effect.";
  html += "</div>";

  // ===============================
  // JavaScript
  // ===============================
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
  html += "    var btns = document.createElement('div'); btns.className = 'modal-buttons';\n";
  html += "    var ok = document.createElement('button'); ok.textContent = 'OK'; ok.className = 'btn btn-primary';\n";
  html += "    ok.onclick = function() { close(); if (onOk) onOk(); };\n";
  html += "    btns.appendChild(ok); box.appendChild(btns);\n";
  html += "  });\n";
  html += "}\n";
  html += "function showConfirm(msg, onYes) {\n";
  html += "  showModal(function(box, close) {\n";
  html += "    box.innerHTML = '<h4>' + msg + '</h4>';\n";
  html += "    var btns = document.createElement('div'); btns.className = 'modal-buttons';\n";
  html += "    var yes = document.createElement('button'); yes.textContent = 'Yes'; yes.className = 'btn btn-success';\n";
  html += "    yes.onclick = function() { close(); onYes(); };\n";
  html += "    var no = document.createElement('button'); no.textContent = 'Cancel'; no.className = 'btn btn-danger';\n";
  html += "    no.onclick = close;\n";
  html += "    btns.appendChild(yes); btns.appendChild(no); box.appendChild(btns);\n";
  html += "  });\n";
  html += "}\n";
  html += "function saveAndReboot(msg) {\n";
  html += "  showAlert(msg + '<br><br>The device will now reboot.', function() {\n";
  html += "    fetch('/api/reboot', {method: 'POST'});\n";
  html += "    document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';\n";
  html += "    setTimeout(function() { location.reload(); }, 10000);\n";
  html += "  });\n";
  html += "}\n";

  // Card 1: Station Info
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

  // Card 2: Hotspot Info
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

  // Save All & Reboot
  html += "function saveAllHotspotSettings() {\n";
  html += "  showConfirm('Save all hotspot settings and reboot?', function() {\n";
  html += "    var post = function(url, body) {\n";
  html += "      return fetch(url, { method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: body });\n";
  html += "    };\n";
  html += "    var callsign = document.getElementById('callsign').value.toUpperCase();\n";
  html += "    var dmrid = document.getElementById('dmrid').value;\n";
  html += "    var ssid = document.getElementById('ssid').value;\n";
  html += "    var hs_callsign = document.querySelector('[name=hs_callsign]').value;\n";
  html += "    var hs_suffix = document.querySelector('[name=hs_suffix]').value;\n";
  html += "    var hs_latitude = document.querySelector('[name=hs_latitude]').value;\n";
  html += "    var hs_longitude = document.querySelector('[name=hs_longitude]').value;\n";
  html += "    var hs_height = document.querySelector('[name=hs_height]').value;\n";
  html += "    var hs_location = document.querySelector('[name=hs_location]').value;\n";
  html += "    var hs_desc = document.querySelector('[name=hs_desc]').value;\n";
  html += "    var hs_url = document.querySelector('[name=hs_url]').value;\n";
  html += "    post('/api/save-station', 'callsign=' + encodeURIComponent(callsign) + '&dmrid=' + dmrid + '&ssid=' + ssid)\n";
  html += "    .then(function() { return post('/api/save-hotspot', 'hs_callsign=' + encodeURIComponent(hs_callsign) + '&hs_suffix=' + encodeURIComponent(hs_suffix) + '&hs_latitude=' + hs_latitude + '&hs_longitude=' + hs_longitude + '&hs_height=' + hs_height + '&hs_location=' + encodeURIComponent(hs_location) + '&hs_desc=' + encodeURIComponent(hs_desc) + '&hs_url=' + encodeURIComponent(hs_url)); })\n";
  html += "    .then(function() {\n";
  html += "      showAlert('All hotspot settings saved.<br><br>The device will now reboot.');\n";
  html += "      fetch('/api/reboot', {method: 'POST'});\n";
  html += "      document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';\n";
  html += "      setTimeout(function() { location.reload(); }, 10000);\n";
  html += "    }).catch(function(err) { showAlert('Error saving settings: ' + err); });\n";
  html += "  });\n";
  html += "}\n";

  html += "</script>\n";

  html += "</div>";
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SYSTEM_HOTSPOT_H
