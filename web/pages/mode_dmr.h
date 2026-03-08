/*
 * DMR Mode Configuration Page
 * Enable/disable DMR mode and configure BrandMeister network connection.
 * Station identity and hotspot location → /system-hotspot
 */

#ifndef WEB_MODE_DMR_H
#define WEB_MODE_DMR_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"
#include "web/include/bm_servers.h"

extern String dmrServer;
extern String dmrPassword;
extern uint32_t dmrRxFreq;
extern uint32_t dmrTxFreq;
extern uint8_t dmrColorCode;
extern uint8_t dmrRfPower;
extern bool modeDmrEnabled;
extern bool dmrServerEspNow;

String getModeDmrPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>DMR Configuration</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("mode-dmr");
  html += "<div class='container'>";
  html += "<h1>DMR Configuration</h1>";
  html += "<p>Configure DMR mode and BrandMeister network. For station identity and location see <a href='/system-hotspot' style='color:var(--link-color);'>Hotspot Config</a>.</p>";
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
  html += "<button class='btn btn-success' onclick='saveDmrMode()'>Save &amp; Reboot</button>";
  html += "<button class='btn btn-danger' onclick='resetDmrMode()'>Reset to Defaults</button>";
  html += "</div>";
  html += "</div>";

  // Card 2: BrandMeister Network / ESP-NOW Relay
  html += "<div class='card'>";
  html += "<h3>DMR Network</h3>";
  html += "<form id='form-network' onsubmit='return saveDmrNetwork(event)'>";
  // Source selector
  html += "<div class='metric' style='margin-bottom:12px;'>";
  html += "<span class='metric-label'>DMR Source:</span>";
  html += "<label style='margin-right:16px;cursor:pointer;'>";
  html += "<input type='radio' name='dmr_source' value='brandmeister'" + String(!dmrServerEspNow ? " checked" : "") + " onchange='updateSourceMode()'> BrandMeister";
  html += "</label>";
  html += "<label style='cursor:pointer;'>";
  html += "<input type='radio' name='dmr_source' value='espnow'" + String(dmrServerEspNow ? " checked" : "") + " onchange='updateSourceMode()'> ESP-NOW Relay";
  html += "</label>";
  html += "</div>";
  bool isKnownServer = false;
  for (int i = 0; i < bmServerCount; i++) {
    if (dmrServer == String(bmServers[i].address)) { isKnownServer = true; break; }
  }
  // BrandMeister-only fields (hidden in ESP-NOW mode)
  html += "<div id='bm-fields'" + String(dmrServerEspNow ? " style='display:none;'" : "") + ">";
  html += "<div class='metric'><span class='metric-label'>Server:</span>";
  html += "<select id='serverSelect' style='width:60%;margin-right:8px;' onchange='updateServerField()'>";
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
  html += "<input type='text' id='serverInput' name='dmr_server' placeholder='IP or FQDN' value='" + dmrServer + "' style='width:38%;display:inline-block;'>";
  html += "</div>";
  html += "<div class='metric'><span class='metric-label'>Password:</span><input type='password' name='dmr_password' value='" + dmrPassword + "'></div>";
  html += "</div>"; // end bm-fields
  html += "<div class='metric'><span class='metric-label'>RX Frequency (Hz):</span><input type='text' name='dmr_rx_freq' value='" + String(dmrRxFreq) + "' required maxlength='10' pattern='\\d{1,10}' title='Enter up to 10 digits'></div>";
  html += "<div class='metric'><span class='metric-label'>TX Frequency (Hz):</span><input type='text' name='dmr_tx_freq' value='" + String(dmrTxFreq) + "' required maxlength='10' pattern='\\d{1,10}' title='Enter up to 10 digits'></div>";
  html += "<div class='metric'><span class='metric-label'>Color Code (1-15):</span><input type='text' name='dmr_color_code' value='" + String(dmrColorCode) + "' required maxlength='2' pattern='1[0-5]|[1-9]' title='Enter 1-15'></div>";
  html += "<div class='metric'><span class='metric-label'>RF Power (0-255):</span><input type='text' name='dmr_rf_power' value='" + String(dmrRfPower) + "' required maxlength='3' pattern='255|[0-9]{1,2}' title='Enter 0-255'></div>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button class='btn btn-success' type='submit'>Save</button>";
  html += "<button class='btn btn-danger' type='button' onclick='resetDmrNetwork()'>Reset to Default</button>";
  html += "</div>";
  html += "</form>";
  html += "<div id='network-status' style='margin-top:10px;padding:10px;display:none;'></div>";
  html += "</div>";

  html += "</div>"; // close admin-grid

  // Save All & Reboot button
  html += "<div class='action-buttons-vertical' style='margin-top:20px;'>";
  html += "<button class='btn btn-success' onclick='saveAllSettings()'>Save All &amp; Reboot</button>";
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

  // Source mode toggle (BrandMeister vs ESP-NOW Relay)
  html += "function updateSourceMode() {\n";
  html += "  var espnow = document.querySelector('[name=dmr_source]:checked').value === 'espnow';\n";
  html += "  document.getElementById('bm-fields').style.display = espnow ? 'none' : '';\n";
  html += "}\n";

  // Server dropdown helper
  html += "function updateServerField() {\n";
  html += "  var sel = document.getElementById('serverSelect');\n";
  html += "  var inp = document.getElementById('serverInput');\n";
  html += "  if (sel.value === 'custom') { inp.value = ''; inp.focus(); } else { inp.value = sel.value; }\n";
  html += "}\n";

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

  // Card 2: DMR Network save/reset
  html += "function saveDmrNetwork(e) {\n";
  html += "  e.preventDefault();\n";
  html += "  var source = document.querySelector('[name=dmr_source]:checked').value;\n";
  html += "  var rxfreq = document.querySelector('[name=dmr_rx_freq]').value;\n";
  html += "  var txfreq = document.querySelector('[name=dmr_tx_freq]').value;\n";
  html += "  var colorcode = document.querySelector('[name=dmr_color_code]').value;\n";
  html += "  var rfpower = document.querySelector('[name=dmr_rf_power]').value;\n";
  html += "  if (!rxfreq || !/^\\d{1,10}$/.test(rxfreq)) { showAlert('RX Frequency must be numeric'); return false; }\n";
  html += "  if (!txfreq || !/^\\d{1,10}$/.test(txfreq)) { showAlert('TX Frequency must be numeric'); return false; }\n";
  html += "  if (!colorcode || colorcode < 1 || colorcode > 15) { showAlert('Color Code must be 1-15'); return false; }\n";
  html += "  var body = 'source=' + source + '&rxfreq=' + rxfreq + '&txfreq=' + txfreq + '&colorcode=' + colorcode + '&rfpower=' + rfpower;\n";
  html += "  var confirmMsg;\n";
  html += "  if (source === 'espnow') {\n";
  html += "    confirmMsg = 'Save DMR network settings?<br><br>Source: ESP-NOW Relay';\n";
  html += "  } else {\n";
  html += "    var server = document.getElementById('serverInput').value;\n";
  html += "    var password = document.querySelector('[name=dmr_password]').value;\n";
  html += "    if (!server) { showAlert('Server address is required'); return false; }\n";
  html += "    if (!password) { showAlert('Password is required'); return false; }\n";
  html += "    body += '&server=' + encodeURIComponent(server) + '&password=' + encodeURIComponent(password);\n";
  html += "    confirmMsg = 'Save DMR network settings?<br><br>Server: ' + server;\n";
  html += "  }\n";
  html += "  showConfirm(confirmMsg, function() {\n";
  html += "    fetch('/api/save-dmr-network', {\n";
  html += "      method: 'POST',\n";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "      body: body\n";
  html += "    }).then(r => r.text()).then(msg => { saveAndReboot(msg); });\n";
  html += "  });\n";
  html += "  return false;\n";
  html += "}\n";
  html += "function resetDmrNetwork() {\n";
  html += "  showConfirm('Reset BrandMeister network settings to defaults?', function() {\n";
  html += "    fetch('/api/reset-dmr-network', {method: 'POST'}).then(r => r.text()).then(msg => { saveAndReboot(msg); });\n";
  html += "  });\n";
  html += "}\n";

  // Save All & Reboot
  html += "function saveAllSettings() {\n";
  html += "  showConfirm('Save all DMR settings and reboot?', function() {\n";
  html += "    var post = function(url, body) {\n";
  html += "      return fetch(url, { method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: body });\n";
  html += "    };\n";
  html += "    var enable = document.getElementById('mode-dmr').checked;\n";
  html += "    var source = document.querySelector('[name=dmr_source]:checked').value;\n";
  html += "    var rxfreq = document.querySelector('[name=dmr_rx_freq]').value;\n";
  html += "    var txfreq = document.querySelector('[name=dmr_tx_freq]').value;\n";
  html += "    var colorcode = document.querySelector('[name=dmr_color_code]').value;\n";
  html += "    var rfpower = document.querySelector('[name=dmr_rf_power]').value;\n";
  html += "    var netBody = 'source=' + source + '&rxfreq=' + rxfreq + '&txfreq=' + txfreq + '&colorcode=' + colorcode + '&rfpower=' + rfpower;\n";
  html += "    if (source !== 'espnow') {\n";
  html += "      var server = document.getElementById('serverInput').value;\n";
  html += "      var password = document.querySelector('[name=dmr_password]').value;\n";
  html += "      netBody += '&server=' + encodeURIComponent(server) + '&password=' + encodeURIComponent(password);\n";
  html += "    }\n";
  html += "    post('/api/mode-toggle', 'mode=dmr&enable=' + enable)\n";
  html += "    .then(function() { return post('/api/save-dmr-network', netBody); })\n";
  html += "    .then(function() {\n";
  html += "      showAlert('All DMR settings saved.<br><br>The device will now reboot.');\n";
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

#endif // WEB_MODE_DMR_H
