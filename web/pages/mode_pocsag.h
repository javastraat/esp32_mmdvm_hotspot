/*
 * POCSAG Mode Configuration Page
 * Configure POCSAG protocol settings
 */

#ifndef WEB_MODE_POCSAG_H
#define WEB_MODE_POCSAG_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

extern bool modePocsagEnabled;
extern bool dapnetEnabled;
extern uint32_t pocsagFrequency;
extern uint32_t userDmrId;
extern String dapnetServer;
extern uint16_t dapnetPort;
extern String dapnetNodeCs;
extern String dapnetAuthKey;
extern uint32_t dapnetRic;
extern String pocsagWhitelist;
extern String pocsagBlacklist;
extern String userCallsign;
extern String hampagerUser;
extern String hampagerPassword;
extern String hampagerTxGroup;

String getModePocsagPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>POCSAG Configuration</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("mode-pocsag");
  html += "<div class='container'>";
  html += "<h1>POCSAG Configuration</h1>";
  html += "<p>Configure POCSAG paging mode settings</p>";
  html += "<div class='admin-grid'>";

  // Resolve RIC once — used by Card 3 and Card 4
  // dapnetRic == 0 means "use own DMR ID as RIC" (convention: 0 is the default/unset value)
  uint32_t ricVal = (dapnetRic > 0) ? dapnetRic : userDmrId;

  // Card 1: POCSAG Mode Status & Save
  html += "<div class='card'>";
  html += "<h3>POCSAG Mode</h3>";
  html += "<div class='metric'><span class='metric-label'>Status:</span><span class='metric-value' id='pocsagStatus'>" + String(modePocsagEnabled ? "Enabled" : "Disabled") + "</span></div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enable POCSAG:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='mode-pocsag'" + String(modePocsagEnabled ? " checked" : "") + ">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='savePocsagSettings()'>Save & Reboot</button>";
  html += "<button class='btn btn-danger' onclick='resetPocsagDefaults()'>Reset to Defaults</button>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>Enable to send POCSAG messages thru API and to use DAPNET</p>";
  html += "</div>";

  // Card 2: Dapnet Configuration (server/network)
  {
    String nodeCs = dapnetNodeCs.length() > 0 ? dapnetNodeCs : userCallsign;

    html += "<div class='card'>";
    html += "<h3>Dapnet Configuration</h3>";

    // Notice banner — hidden when POCSAG is enabled, visible when not
    html += "<div id='dapnet-pocsag-notice' style='background:#fff3cd;border:1px solid #ffc107;border-radius:6px;padding:8px 12px;margin-bottom:10px;font-size:0.88em;color:#856404;" + String(modePocsagEnabled ? "display:none;" : "") + "'>";
    html += "&#9888; Enable POCSAG first (Card 1) before activating DAPNET.";
    html += "</div>";

    // Dapnet Enable toggle — disabled when POCSAG is off
    html += "<div class='metric'>";
    html += "<span class='metric-label'>Enable Dapnet Client:</span>";
    html += "<label class='switch'>";
    html += "<input type='checkbox' id='dapnet-enable'" + String(dapnetEnabled ? " checked" : "") + String(!modePocsagEnabled ? " disabled" : "") + ">";
    html += "<span class='slider'></span>";
    html += "</label>";
    html += "</div>";

    // Dapnet Server dropdown
    html += "<div class='metric' style='position:relative;'>";
    html += "<span class='metric-label'>Dapnet Server:</span>";
    html += "<select id='dapnet_server' style='width: 120px;'>";
    const char* servers[] = {
      "dapnet.afu.rwth-aachen.de",
      "137.226.79.100",
      "db0dbn.ig-funk-siebengebirge.de",
      "dapnet.db0sda.ampr.org",
      "node1.dapnet-italia.it"
    };
    for (int i = 0; i < 4; i++) {
      bool sel = (dapnetServer == String(servers[i]));
      html += "<option value='" + String(servers[i]) + "'" + (sel ? " selected" : "") + ">" + String(servers[i]) + "</option>";
    }
    html += "</select>";
    html += "</div>";

    // Dapnet Port
    html += "<div class='metric' style='position:relative;'>";
    html += "<span class='metric-label'>Dapnet Port:</span>";
    html += "<input type='text' id='dapnet_port' value='" + String(dapnetPort) + "' maxlength='5' style='width: 120px; padding-right: 8px;' oninput=\"this.value=this.value.replace(/[^0-9]/g,'');\" onblur=\"if(!this.value||parseInt(this.value)<1)this.value='43434';\">";
    html += "</div>";

    // Dapnet Callsign
    html += "<div class='metric' style='position:relative;'>";
    html += "<span class='metric-label'>Dapnet Callsign:</span>";
    html += "<input type='text' id='dapnet_cs' value='" + nodeCs + "' maxlength='16' style='width: 120px; padding-right: 8px;'>";
    html += "</div>";

    // Dapnet AuthKey
    html += "<div class='metric' style='position:relative;'>";
    html += "<span class='metric-label'>Dapnet AuthKey:</span>";
    html += "<input type='password' id='dapnet_key' value='" + dapnetAuthKey + "' maxlength='64' style='width: 120px; padding-right: 8px;'>";
    html += "</div>";

    html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
    html += "<button class='btn btn-success' onclick='saveDapnetSettings()'>Save</button>";
    html += "<button class='btn btn-danger' onclick='resetPocsagNetSettings()'>Reset to Defaults</button>";
    html += "</div>";
    html += "<div id='dapnetResult' style='margin-top:8px;font-size:0.9em;'></div>";
    html += "</div>";
  }

  // Card 3: HamPager Credentials
  {
    html += "<div class='card'>";
    html += "<h3>HamPager</h3>";
    html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Credentials for the <a href='http://hampager.de' target='_blank' style='color:#007bff;'>HamPager</a> REST API.</p>";

    html += "<div class='metric' style='position:relative;'>";
    html += "<span class='metric-label'>Username:</span>";
    html += "<input type='text' id='hp_user' value='" + hampagerUser + "' maxlength='32' autocomplete='username' style='width: 130px; padding-right: 8px;' placeholder='your-callsign'>";
    html += "</div>";

    html += "<div class='metric' style='position:relative;'>";
    html += "<span class='metric-label'>Password:</span>";
    html += "<input type='password' id='hp_pass' value='" + hampagerPassword + "' maxlength='64' autocomplete='current-password' style='width: 130px; padding-right: 8px;' placeholder='hampager password'>";
    html += "</div>";

    html += "<div class='metric' style='position:relative;'>";
    html += "<span class='metric-label'>TX Group:</span>";
    html += "<input type='text' id='hp_txg' value='" + hampagerTxGroup + "' maxlength='32' style='width: 130px; padding-right: 8px;' placeholder='all'>";
    html += "</div>";
    html += "<p style='font-size:0.80em;color:#888;margin-top:2px;margin-bottom:8px;'>E.g. <code>all</code>, <code>pa-nh</code>, <code>dl-all</code></p>";

    html += "<div class='action-buttons-vertical' style='margin-top:12px;'>";
    html += "<button class='btn btn-success' onclick='saveHampagerSettings()'>Save Credentials</button>";
    html += "<button class='btn btn-danger' onclick='resetHampagerSettings()'>Reset to Defaults</button>";
    html += "</div>";
    html += "<div id='hampagerSaveResult' style='margin-top:6px;font-size:0.9em;'></div>";

    html += "</div>"; // card
  }

  // Card 4: Send DAPNET Message via HamPager
  {
    html += "<div class='card'>";
    html += "<h3>Send DAPNET Message</h3>";
    html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Send a paging message to any callsign via HamPager.</p>";

    html += "<div class='metric' style='position:relative;'>";
    html += "<span class='metric-label'>To Callsign:</span>";
    html += "<input type='text' id='hp_dest' value='" + userCallsign + "' maxlength='16' style='width: 130px; padding-right: 8px;' placeholder='N0CALL'>";
    html += "</div>";

    html += "<div class='metric' style='position:relative;'>";
    html += "<span class='metric-label'>TX Group:</span>";
    html += "<input type='text' id='hp_send_txg' value='" + hampagerTxGroup + "' maxlength='32' style='width: 130px; padding-right: 8px;' placeholder='all'>";
    html += "</div>";

    html += "<div class='metric' style='position:relative;'>";
    html += "<span class='metric-label'>Message:</span>";
    html += "<input type='text' id='hp_msg' value='' maxlength='160' style='width: 200px; padding-right: 8px;' placeholder='Your message here'>";
    html += "</div>";

    html += "<div class='action-buttons-vertical' style='margin-top:12px;'>";
    html += "<button class='btn btn-primary' onclick='sendHampager()'>Send via HamPager</button>";
    html += "</div>";
    html += "<div id='hampagerSendResult' style='margin-top:6px;font-size:0.9em;'></div>";

    html += "</div>"; // card
  }

  // Card 5: POCSAG Configuration (modem)
  {
    html += "<div class='card'>";
    html += "<h3>POCSAG Configuration</h3>";

    // Frequency (MHz)
    html += "<div class='metric' style='position:relative;'>";
    html += "<span class='metric-label'>Frequency (MHz):</span>";
    html += "<input type='text' id='pocsag_freq_mhz' value='" + String(pocsagFrequency / 1000000.0, 6) + "' style='width: 120px; padding-right: 8px;' oninput=\"this.value=this.value.replace(/[^0-9.]/g,'');\" placeholder='439.987500'>";
    html += "</div>";

    // RIC
    html += "<div class='metric' style='position:relative;'>";
    html += "<span class='metric-label'>RIC:</span>";
    html += "<input type='text' id='dapnet_ric' value='" + String(ricVal) + "' maxlength='7' style='width: 120px; padding-right: 8px;' oninput=\"this.value=this.value.replace(/[^0-9]/g,'');if(this.value.length>7)this.value=this.value.slice(0,7);\" onblur=\"if(parseInt(this.value)<1||this.value==='')this.value='1';if(parseInt(this.value)>9999999)this.value='9999999';\">";
    html += "</div>";
    html += "<p style='font-size:0.85em;color:#666;margin-top:2px;margin-bottom:6px;'>7-digit Receiver Identity Code</p>";

    // Whitelist
    html += "<div class='metric' style='position:relative;'>";
    html += "<span class='metric-label'>Whitelist:</span>";
    html += "<input type='text' id='pocsag_wlist' value='" + pocsagWhitelist + "' placeholder='RIC1,RIC2,...' style='width: 120px; padding-right: 8px;'>";
    html += "</div>";

    // Blacklist
    html += "<div class='metric' style='position:relative;'>";
    html += "<span class='metric-label'>Blacklist:</span>";
    html += "<input type='text' id='pocsag_blist' value='" + pocsagBlacklist + "' placeholder='RIC1,RIC2,...' style='width: 120px; padding-right: 8px;'>";
    html += "</div>";

    html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
    html += "<button class='btn btn-success' onclick='savePocsagModemSettings()'>Save</button>";
    html += "<button class='btn btn-danger' onclick='resetPocsagNetSettings()'>Reset to Defaults</button>";
    html += "</div>";
    html += "<div id='pocsagModemResult' style='margin-top:8px;font-size:0.9em;'></div>";
    html += "</div>";
  }

  // Card 5: Send Test POCSAG Message
  html += "<div class='card'>";
  html += "<h3>Send Test POCSAG Message</h3>";
  html += "<form id='pocsagTestForm' onsubmit='return sendPocsagTest(event)'>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>RIC:</span>";
  html += "<input type='text' id='pocsagRic' name='ric' placeholder='" + String(ricVal) + "' value='" + String(ricVal) + "' maxlength='7' required style='width: 120px; padding-right: 8px;' oninput=\"this.value=this.value.replace(/[^0-9]/g,'');if(this.value.length>7)this.value=this.value.slice(0,7);\" onblur=\"if(parseInt(this.value)<1||this.value==='')this.value='1';if(parseInt(this.value)>9999999)this.value='9999999';\">";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Encoding:</span>";
  html += "<select id='pocsagEnc' name='encoding' style='width: 120px;'>";
  html += "<option value='alpha' selected>Alphanumeric</option>";
  html += "<option value='numeric'>Numeric</option>";
  html += "</select>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Message:</span>";
  html += "<input type='text' id='pocsagMsg' name='message' placeholder='Test message' value='Test message' maxlength='80' required style='width: 200px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button type='submit' class='btn btn-success'>Send Message</button>";
  html += "</div>";
  html += "<div id='pocsagTestResult' style='margin-top:10px;font-size:0.95em;'></div>";
  html += "</form>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>Frequency: <b>" + String(pocsagFrequency / 1000000.0, 6) + " MHz</b></p>";
  html += "</div>";

  // Card 6: POCSAG Queue Monitor
  html += "<div class='card'>";
  html += "<h3>POCSAG Queue</h3>";
  html += "<div class='metric'><span class='metric-label'>Pending:</span><span class='metric-value' id='queueCount'>-</span></div>";
  html += "<div id='queueTable' style='margin-top:10px;'></div>";
  html += "<div class='action-buttons-vertical' style='margin-top:10px;'>";
  html += "<button class='btn btn-primary' onclick='refreshQueue()'>Refresh</button>";
  html += "</div>";
  html += "</div>";

  // Card 7: DAPNET Received Message History
  html += "<div class='card'>";
  html += "<h3>Last Received Messages</h3>";
  html += "<div id='historyTable' style='margin-top:6px;'><p style='color:#888;font-size:0.9em;'>Loading...</p></div>";
  html += "<div class='action-buttons-vertical' style='margin-top:10px;'>";
  html += "<button class='btn btn-primary' onclick='refreshHistory()'>Refresh</button>";
  html += "</div>";
  html += "</div>";

  html += "</div>"; // close admin-grid

  // Save All & Reboot button
  html += "<div class='action-buttons-vertical' style='margin-top:20px;'>";
  html += "<button class='btn btn-success' onclick='saveAllPocsagSettings()'>Save All &amp; Reboot</button>";
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

  // Card 1: Save & Reboot
  html += "function savePocsagSettings() {\n";
  html += "  var enable = document.getElementById('mode-pocsag').checked;\n";
  html += "  showConfirm('Save POCSAG settings and reboot?', function() {\n";
  html += "    fetch('/api/mode-toggle', {\n";
  html += "      method: 'POST',\n";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "      body: 'mode=pocsag&enable=' + enable\n";
  html += "    }).then(r => r.text()).then(msg => { saveAndReboot(msg); })\n";
  html += "      .catch(function() { showAlert('Failed to save settings.'); });\n";
  html += "  });\n";
  html += "}\n";
  html += "function resetPocsagDefaults() {\n";
  html += "  showConfirm('Reset POCSAG settings to defaults and reboot?', function() {\n";
  html += "    fetch('/api/mode-toggle', {\n";
  html += "      method: 'POST',\n";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "      body: 'mode=pocsag&enable=false'\n";
  html += "    }).then(r => r.text()).then(msg => { saveAndReboot(msg); })\n";
  html += "      .catch(function() { showAlert('Failed to reset settings.'); });\n";
  html += "  });\n";
  html += "}\n";

  // Card 2: Send test message
  html += "function sendPocsagTest(e) {\n";
  html += "  e.preventDefault();\n";
  html += "  var ric = document.getElementById('pocsagRic').value;\n";
  html += "  var msg = document.getElementById('pocsagMsg').value;\n";
  html += "  var enc = document.getElementById('pocsagEnc').value;\n";
  html += "  var result = document.getElementById('pocsagTestResult');\n";
  html += "  result.innerHTML = 'Sending...';\n";
  html += "  fetch('/api/send-pocsag', {\n";
  html += "    method: 'POST',\n";
  html += "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },\n";
  html += "    body: 'ric=' + encodeURIComponent(ric) + '&message=' + encodeURIComponent(msg) + '&encoding=' + enc\n";
  html += "  })\n";
  html += "  .then(r => r.json())\n";
  html += "  .then(j => {\n";
  html += "    if (j.success) result.innerHTML = '<span style=\\'color:green\\'>Message sent!</span>';\n";
  html += "    else result.innerHTML = '<span style=\\'color:red\\'>Error: ' + (j.error || 'Unknown error') + '</span>';\n";
  html += "  })\n";
  html += "  .catch(e => { result.innerHTML = '<span style=\\'color:red\\'>Request failed</span>'; });\n";
  html += "  return false;\n";
  html += "}\n";

  // Card 2: Save Dapnet settings (enable toggle + network config), then reboot
  html += "function saveDapnetSettings() {\n";
  html += "  var enable = document.getElementById('dapnet-enable').checked;\n";
  html += "  var body = 'dapnet_server=' + encodeURIComponent(document.getElementById('dapnet_server').value)\n";
  html += "    + '&dapnet_port=' + encodeURIComponent(document.getElementById('dapnet_port').value)\n";
  html += "    + '&dapnet_cs=' + encodeURIComponent(document.getElementById('dapnet_cs').value)\n";
  html += "    + '&dapnet_key=' + encodeURIComponent(document.getElementById('dapnet_key').value);\n";
  html += "  showConfirm('Save DAPNET settings and reboot?', function() {\n";
  html += "    fetch('/api/mode-toggle', {\n";
  html += "      method: 'POST',\n";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "      body: 'mode=dapnet&enable=' + enable\n";
  html += "    }).then(() => fetch('/api/save-pocsag-settings', {\n";
  html += "      method: 'POST',\n";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "      body: body\n";
  html += "    })).then(r => r.text()).then(msg => { saveAndReboot(msg); })\n";
  html += "      .catch(function() { showAlert('Failed to save settings.'); });\n";
  html += "  });\n";
  html += "}\n";

  // Card 5: Save POCSAG modem settings
  html += "function savePocsagModemSettings() {\n";
  html += "  var freqHz = Math.round(parseFloat(document.getElementById('pocsag_freq_mhz').value) * 1000000);\n";
  html += "  var body = 'dapnet_ric=' + encodeURIComponent(document.getElementById('dapnet_ric').value)\n";
  html += "    + '&pocsag_freq=' + encodeURIComponent(freqHz)\n";
  html += "    + '&pocsag_wlist=' + encodeURIComponent(document.getElementById('pocsag_wlist').value)\n";
  html += "    + '&pocsag_blist=' + encodeURIComponent(document.getElementById('pocsag_blist').value);\n";
  html += "  fetch('/api/save-pocsag-settings', { method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: body })\n";
  html += "    .then(r => r.text()).then(msg => { document.getElementById('pocsagModemResult').innerHTML = '<span style=\\'color:green\\'>'+msg+'</span>'; })\n";
  html += "    .catch(() => { document.getElementById('pocsagModemResult').innerHTML = '<span style=\\'color:red\\'>Save failed</span>'; });\n";
  html += "}\n";

  // Card 3: HamPager save/reset/send
  html += "function saveHampagerSettings() {\n";
  html += "  var body = 'hp_user=' + encodeURIComponent(document.getElementById('hp_user').value)\n";
  html += "    + '&hp_pass=' + encodeURIComponent(document.getElementById('hp_pass').value)\n";
  html += "    + '&hp_txg='  + encodeURIComponent(document.getElementById('hp_txg').value);\n";
  html += "  fetch('/api/save-hampager-settings', { method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: body })\n";
  html += "    .then(r => r.text()).then(msg => { document.getElementById('hampagerSaveResult').innerHTML = '<span style=\\'color:green\\'>'+msg+'</span>'; })\n";
  html += "    .catch(() => { document.getElementById('hampagerSaveResult').innerHTML = '<span style=\\'color:red\\'>Save failed</span>'; });\n";
  html += "}\n";
  html += "function resetHampagerSettings() {\n";
  html += "  showConfirm('Reset HamPager settings to defaults?', function() {\n";
  html += "    fetch('/api/reset-hampager-settings', { method: 'POST' })\n";
  html += "      .then(r => r.text()).then(msg => { document.getElementById('hampagerSaveResult').innerHTML = '<span style=\\'color:green\\'>'+msg+'</span>'; })\n";
  html += "      .catch(() => { document.getElementById('hampagerSaveResult').innerHTML = '<span style=\\'color:red\\'>Reset failed</span>'; });\n";
  html += "  });\n";
  html += "}\n";
  html += "function sendHampager() {\n";
  html += "  var dest = document.getElementById('hp_dest').value.trim();\n";
  html += "  var msg  = document.getElementById('hp_msg').value.trim();\n";
  html += "  var txg  = document.getElementById('hp_send_txg').value.trim();\n";
  html += "  var result = document.getElementById('hampagerSendResult');\n";
  html += "  if (!dest || !msg) { result.innerHTML = '<span style=\\'color:red\\'>Callsign and message are required</span>'; return; }\n";
  html += "  result.innerHTML = 'Sending...';\n";
  html += "  var body = 'callsign=' + encodeURIComponent(dest)\n";
  html += "    + '&text=' + encodeURIComponent(msg)\n";
  html += "    + '&txg='  + encodeURIComponent(txg);\n";
  html += "  fetch('/api/send-hampager', { method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: body })\n";
  html += "    .then(function(r) {\n";
  html += "      return r.json().then(function(j) {\n";
  html += "        if (r.ok) {\n";
  html += "          result.innerHTML = '<span style=\\'color:green\\'>Sent to ' + dest + '!</span>';\n";
  html += "        } else {\n";
  html += "          var errMsg = 'Send failed';\n";
  html += "          if (j.violations && j.violations.length > 0) {\n";
  html += "            var v = j.violations[0];\n";
  html += "            if (v.constraint === 'ValidCallSignNames') {\n";
  html += "              errMsg = 'Callsign <b>' + dest + '</b> is not registered in DAPNET.';\n";
  html += "            } else {\n";
  html += "              errMsg = v.message || j.message || 'Unknown error';\n";
  html += "            }\n";
  html += "          } else if (j.message) {\n";
  html += "            errMsg = j.message;\n";
  html += "          }\n";
  html += "          result.innerHTML = '<span style=\\'color:red\\'>' + errMsg + '</span>';\n";
  html += "          showAlert(errMsg);\n";
  html += "        }\n";
  html += "      });\n";
  html += "    })\n";
  html += "    .catch(function() { result.innerHTML = '<span style=\\'color:red\\'>Request failed</span>'; });\n";
  html += "}\n";

  // Cards 5 & 6: Reset all POCSAG/Dapnet settings to defaults
  html += "function resetPocsagNetSettings() {\n";
  html += "  showConfirm('Reset all POCSAG & Dapnet settings to defaults?', function() {\n";
  html += "    fetch('/api/reset-pocsag-settings', { method: 'POST' })\n";
  html += "      .then(r => r.text()).then(msg => { showAlert(msg + '<br>Reload to see defaults.'); })\n";
  html += "      .catch(() => { showAlert('Reset failed.'); });\n";
  html += "  });\n";
  html += "}\n";

  // Card 6: POCSAG Queue Monitor
  html += "var queueRefreshTimer = null;\n";
  html += "function refreshQueue() {\n";
  html += "  fetch('/api/pocsag-queue')\n";
  html += "    .then(r => r.json())\n";
  html += "    .then(function(d) {\n";
  html += "      document.getElementById('queueCount').textContent = d.count + ' / ' + d.capacity;\n";
  html += "      var tbl = document.getElementById('queueTable');\n";
  html += "      if (d.count === 0) { tbl.innerHTML = '<p style=\\'color:#888;font-size:0.9em;\\'>Queue is empty</p>'; return; }\n";
  html += "      var html = '<table style=\\'width:100%;font-size:0.85em;border-collapse:collapse;\\'>';\n";
  html += "      html += '<tr><th style=\\'text-align:left;padding:3px;border-bottom:1px solid #ccc;\\'>RIC</th>';\n";
  html += "      html += '<th style=\\'text-align:left;padding:3px;border-bottom:1px solid #ccc;\\'>Type</th>';\n";
  html += "      html += '<th style=\\'text-align:left;padding:3px;border-bottom:1px solid #ccc;\\'>Message</th></tr>';\n";
  html += "      var types = ['Numeric','Alert1','Alert2','Alpha'];\n";
  html += "      for (var i = 0; i < d.items.length; i++) {\n";
  html += "        var item = d.items[i];\n";
  html += "        html += '<tr><td style=\\'padding:3px;\\'>' + item.ric + '</td>';\n";
  html += "        html += '<td style=\\'padding:3px;\\'>' + (types[item.func] || item.func) + '</td>';\n";
  html += "        html += '<td style=\\'padding:3px;word-break:break-all;\\'>' + item.msg + '</td></tr>';\n";
  html += "      }\n";
  html += "      html += '</table>';\n";
  html += "      tbl.innerHTML = html;\n";
  html += "    })\n";
  html += "    .catch(function() { document.getElementById('queueCount').textContent = 'error'; });\n";
  html += "}\n";
  html += "refreshQueue();\n";
  html += "queueRefreshTimer = setInterval(refreshQueue, 2000);\n";

  // Card 7: DAPNET message history
  html += "var histRefreshTimer = null;\n";
  html += "function refreshHistory() {\n";
  html += "  fetch('/api/dapnet-history')\n";
  html += "    .then(r => r.json())\n";
  html += "    .then(function(d) {\n";
  html += "      var tbl = document.getElementById('historyTable');\n";
  html += "      if (d.count === 0) { tbl.innerHTML = '<p style=\\'color:#888;font-size:0.9em;\\'>No messages received yet</p>'; return; }\n";
  html += "      var types = ['Numeric','Alert1','Alert2','Alpha'];\n";
  html += "      var h = '<table style=\\'width:100%;font-size:0.85em;border-collapse:collapse;\\'>';\n";
  html += "      h += '<tr>';\n";
  html += "      h += '<th style=\\'text-align:left;padding:3px;border-bottom:1px solid #ccc;\\'>Time</th>';\n";
  html += "      h += '<th style=\\'text-align:left;padding:3px;border-bottom:1px solid #ccc;\\'>RIC</th>';\n";
  html += "      h += '<th style=\\'text-align:left;padding:3px;border-bottom:1px solid #ccc;\\'>Type</th>';\n";
  html += "      h += '<th style=\\'text-align:left;padding:3px;border-bottom:1px solid #ccc;\\'>Message</th></tr>';\n";
  html += "      for (var i = d.items.length - 1; i >= 0; i--) {\n";
  html += "        var item = d.items[i];\n";
  html += "        h += '<tr>';\n";
  html += "        h += '<td style=\\'padding:3px;white-space:nowrap;\\'>' + item.time + '</td>';\n";
  html += "        h += '<td style=\\'padding:3px;\\'>' + item.ric + '</td>';\n";
  html += "        h += '<td style=\\'padding:3px;\\'>' + (types[item.func] || 'f' + item.func) + '</td>';\n";
  html += "        h += '<td style=\\'padding:3px;word-break:break-all;\\'>' + item.msg + '</td>';\n";
  html += "        h += '</tr>';\n";
  html += "      }\n";
  html += "      h += '</table>';\n";
  html += "      tbl.innerHTML = h;\n";
  html += "    })\n";
  html += "    .catch(function() { document.getElementById('historyTable').innerHTML = '<p style=\\'color:red;font-size:0.9em;\\'>Error loading history</p>'; });\n";
  html += "}\n";
  html += "refreshHistory();\n";
  html += "histRefreshTimer = setInterval(refreshHistory, 10000);\n";

  // Save All & Reboot — collects all cards and posts sequentially then reboots
  html += "function saveAllPocsagSettings() {\n";
  html += "  showConfirm('Save all POCSAG settings and reboot?', function() {\n";
  html += "    var post = function(url, body) {\n";
  html += "      return fetch(url, { method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: body });\n";
  html += "    };\n";
  html += "    var pocsagOn = document.getElementById('mode-pocsag').checked;\n";
  html += "    var dapnetOn = document.getElementById('dapnet-enable').checked;\n";
  html += "    var freqHz   = Math.round(parseFloat(document.getElementById('pocsag_freq_mhz').value) * 1000000);\n";
  html += "    var body = 'pocsag_freq=' + encodeURIComponent(freqHz)\n";
  html += "      + '&dapnet_server=' + encodeURIComponent(document.getElementById('dapnet_server').value)\n";
  html += "      + '&dapnet_port='   + encodeURIComponent(document.getElementById('dapnet_port').value)\n";
  html += "      + '&dapnet_cs='     + encodeURIComponent(document.getElementById('dapnet_cs').value)\n";
  html += "      + '&dapnet_key='    + encodeURIComponent(document.getElementById('dapnet_key').value)\n";
  html += "      + '&dapnet_ric='    + encodeURIComponent(document.getElementById('dapnet_ric').value)\n";
  html += "      + '&pocsag_wlist='  + encodeURIComponent(document.getElementById('pocsag_wlist').value)\n";
  html += "      + '&pocsag_blist='  + encodeURIComponent(document.getElementById('pocsag_blist').value);\n";
  html += "    var hpBody = 'hp_user=' + encodeURIComponent(document.getElementById('hp_user').value)\n";
  html += "      + '&hp_pass=' + encodeURIComponent(document.getElementById('hp_pass').value)\n";
  html += "      + '&hp_txg='  + encodeURIComponent(document.getElementById('hp_txg').value);\n";
  html += "    post('/api/mode-toggle', 'mode=pocsag&enable=' + pocsagOn)\n";
  html += "      .then(function() { return post('/api/mode-toggle', 'mode=dapnet&enable=' + dapnetOn); })\n";
  html += "      .then(function() { return post('/api/save-pocsag-settings', body); })\n";
  html += "      .then(function() { return post('/api/save-hampager-settings', hpBody); })\n";
  html += "      .then(function() { saveAndReboot('All POCSAG settings saved.'); })\n";
  html += "      .catch(function() { showAlert('Failed to save settings.'); });\n";
  html += "  });\n";
  html += "}\n";

  // Reactively enable/disable DAPNET toggle based on POCSAG Card 1 state
  html += "function updateDapnetToggleState() {\n";
  html += "  var pocsagOn = document.getElementById('mode-pocsag').checked;\n";
  html += "  var notice = document.getElementById('dapnet-pocsag-notice');\n";
  html += "  var dapnetToggle = document.getElementById('dapnet-enable');\n";
  html += "  if (pocsagOn) {\n";
  html += "    notice.style.display = 'none';\n";
  html += "    dapnetToggle.disabled = false;\n";
  html += "  } else {\n";
  html += "    notice.style.display = '';\n";
  html += "    dapnetToggle.disabled = true;\n";
  html += "    dapnetToggle.checked = false;\n";
  html += "  }\n";
  html += "}\n";
  html += "document.getElementById('mode-pocsag').addEventListener('change', updateDapnetToggleState);\n";

  html += "</script>\n";

  html += "<div class='info' style='margin-top:20px'>";
  html += "<strong>Note:</strong> Use <b>Save All &amp; Reboot</b> to save all cards at once, or use the individual Save buttons on each card.";
  html += "</div>";

  html += "</div>";
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_MODE_POCSAG_H
