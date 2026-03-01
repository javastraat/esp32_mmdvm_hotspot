/*
 * MMDVM Settings Page
 * Configure MMDVM hardware settings (frequency, color code, etc)
 */

#ifndef WEB_SETTINGS_MMDVM_H
#define WEB_SETTINGS_MMDVM_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

// External references to runtime settings
extern String userCallsign;
extern uint32_t userDmrId;
extern uint8_t userDmrSsid;
extern uint32_t dmrRxFreq;
extern uint32_t dmrTxFreq;
extern uint8_t dmrColorCode;
extern uint8_t dmrRfPower;
extern bool cwidEnabled;
extern uint8_t cwidIntervalMin;

// MMDVM modem pin and baudrate variables
extern int mmdvmRxPin;
extern int mmdvmTxPin;
extern int mmdvmBootPin;
extern int mmdvmResetPin;
extern int mmdvmWakeupPin;
extern int mmdvmBaudrate;

String getSettingsMmdvmPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>MMDVM Settings</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("settings-mmdvm");

  html += "<div class='container'>";
  html += "<h1>MMDVM Settings</h1>";
  html += "<p>Configure MMDVM modem hardware and station parameters</p>";

  html += "<div class='admin-grid'>";

  // Card 1: Station Information card
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

  // Card 2: RF Settings
  html += "<div class='card'>";
  html += "<h3>RF Settings</h3>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>RX Freq (Hz):</span>";
  html += "<input type='text' id='rx-freq' value='" + String(dmrRxFreq) + "' inputmode='numeric' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>TX Freq (Hz):</span>";
  html += "<input type='text' id='tx-freq' value='" + String(dmrTxFreq) + "' inputmode='numeric' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Color Code (0-15):</span>";
  html += "<input type='text' id='color-code' value='" + String(dmrColorCode) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>RF Power (0-255):</span>";
  html += "<input type='text' id='rf-power' value='" + String(dmrRfPower) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "Frequency in Hz (e.g. 434000000 for 434 MHz). Color Code typically 1.";
  html += "</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveRfSettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetRfSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 4: Modem Settings
  html += "<div class='card'>";
  html += "<h3>Modem Settings</h3>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Serial Baudrate:</span>";
  html += "<input type='text' id='mmdvm-baudrate' value='" + String(mmdvmBaudrate) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<details style='margin-top:10px;'>";
  html += "<summary style='cursor:pointer;color:#007bff;font-size:0.9em;'>Advanced Pin Configuration</summary>";
  html += "<div style='margin-top:8px;'>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>MMDVM RX Pin:</span>";
  html += "<input type='text' id='mmdvm-rx-pin' value='" + String(mmdvmRxPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>MMDVM TX Pin:</span>";
  html += "<input type='text' id='mmdvm-tx-pin' value='" + String(mmdvmTxPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>MMDVM BOOT Pin:</span>";
  html += "<input type='text' id='mmdvm-boot-pin' value='" + String(mmdvmBootPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>MMDVM RESET Pin:</span>";
  html += "<input type='text' id='mmdvm-reset-pin' value='" + String(mmdvmResetPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>MMDVM WAKEUP Pin:</span>";
  html += "<input type='text' id='mmdvm-wakeup-pin' value='" + String(mmdvmWakeupPin) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "</div>";
  html += "</details>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "Configure MMDVM modem serial pins and baudrate.";
  html += "</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveModemSettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetModemSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // Card 5: Timeouts
  html += "<div class='card'>";
  html += "<h3>Timeouts</h3>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>TX Hang (ms):</span>";
  html += "<input type='text' id='tx-hang' value='1000' disabled style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Mode Hang (ms):</span>";
  html += "<input type='text' id='mode-hang' value='1000' disabled style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Net Timeout (s):</span>";
  html += "<input type='text' id='net-timeout' value='60' disabled style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "Timeout settings (placeholder - coming soon).";
  html += "</p>";
  html += "</div>";

  // Card 6: Advanced Settings
  html += "<div class='card'>";
  html += "<h3>Advanced Settings</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Debug Mode:</span>";
  html += "<label class='switch'><input type='checkbox' id='debug-mode' disabled><span class='slider'></span></label>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>OLED Display:</span>";
  html += "<select id='oled-mode' disabled style='width: 120px;'>";
  html += "<option value='auto' selected>Auto</option>";
  html += "<option value='status'>Status</option>";
  html += "<option value='network'>Network</option>";
  html += "</select>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "Advanced settings (placeholder - coming soon).";
  html += "</p>";
  html += "</div>";

  // Card 7: CW ID
  html += "<div class='card'>";
  html += "<h3>CW ID</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enable CW ID:</span>";
  html += "<label class='switch'><input type='checkbox' id='cwid-enabled'" + String(cwidEnabled ? " checked" : "") + "><span class='slider'></span></label>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Interval (min):</span>";
  html += "<input type='number' id='cwid-interval' value='" + String(cwidIntervalMin) + "' min='1' max='60' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>";
  html += "Transmits your callsign (<b>" + userCallsign + "</b>) in Morse code at the set interval for regulatory compliance. ";
  html += "Uses the DMR TX frequency, or the POCSAG frequency when POCSAG-only mode is active.";
  html += "</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveCwidSettings()'>Save</button>";
  html += "<button class='btn btn-primary' onclick='testCwid()'>Test Now</button>";
  html += "<button class='btn btn-danger' onclick='resetCwidSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  html += "<div class='info' style='margin-top:20px'>";
  html += "<strong>Note:</strong> MMDVM modem settings interface is a placeholder. Hardware configuration will be implemented in future updates.";
  html += "</div>";

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
  // Station Info
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
  // RF Settings
  html += "function saveRfSettings() {\n";
  html += "  var rxFreq = document.getElementById('rx-freq').value;\n";
  html += "  var txFreq = document.getElementById('tx-freq').value;\n";
  html += "  var colorCode = document.getElementById('color-code').value;\n";
  html += "  var rfPower = document.getElementById('rf-power').value;\n";
  html += "  if (!rxFreq || !/^[0-9]{8,9}$/.test(rxFreq)) { showAlert('RX Frequency must be 8-9 digits (Hz)'); return; }\n";
  html += "  if (!txFreq || !/^[0-9]{8,9}$/.test(txFreq)) { showAlert('TX Frequency must be 8-9 digits (Hz)'); return; }\n";
  html += "  if (colorCode < 0 || colorCode > 15) { showAlert('Color Code must be 0-15'); return; }\n";
  html += "  if (rfPower < 0 || rfPower > 255) { showAlert('RF Power must be 0-255'); return; }\n";
  html += "  showConfirm('Save RF settings?<br>RX: ' + rxFreq + ' Hz<br>TX: ' + txFreq + ' Hz<br>CC: ' + colorCode + '<br>Power: ' + rfPower, function() {\n";
  html += "    fetch('/api/save-rf-settings', {\n";
  html += "      method: 'POST',\n";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "      body: 'rxfreq=' + rxFreq + '&txfreq=' + txFreq + '&colorcode=' + colorCode + '&rfpower=' + rfPower\n";
  html += "    }).then(r => r.text()).then(msg => { saveAndReboot(msg); });\n";
  html += "  });\n";
  html += "}\n";
  html += "function resetRfSettings() {\n";
  html += "  showConfirm('Reset RF settings to defaults?<br><br>RX Freq: " + String(DMR_RX_FREQ) + " Hz<br>TX Freq: " + String(DMR_TX_FREQ) + " Hz<br>Color Code: " + String(DMR_COLOR_CODE) + "<br>RF Power: " + String(DMR_RF_POWER) + "', function() {\n";
  html += "    fetch('/api/reset-rf-settings', {method: 'POST'}).then(r => r.text()).then(msg => { saveAndReboot(msg); });\n";
  html += "  });\n";
  html += "}\n";
  // Modem Settings
  html += "function saveModemSettings() {\n";
  html += "  var rxPin = document.getElementById('mmdvm-rx-pin').value;\n";
  html += "  var txPin = document.getElementById('mmdvm-tx-pin').value;\n";
  html += "  var bootPin = document.getElementById('mmdvm-boot-pin').value;\n";
  html += "  var resetPin = document.getElementById('mmdvm-reset-pin').value;\n";
  html += "  var wakeupPin = document.getElementById('mmdvm-wakeup-pin').value;\n";
  html += "  var baudrate = document.getElementById('mmdvm-baudrate').value;\n";
  html += "  showConfirm('Save modem settings?<br>RX Pin: ' + rxPin + '<br>TX Pin: ' + txPin + '<br>BOOT Pin: ' + bootPin + '<br>RESET Pin: ' + resetPin + '<br>WAKEUP Pin: ' + wakeupPin + '<br>Baudrate: ' + baudrate, function() {\n";
  html += "    fetch('/api/save-modem-settings', {\n";
  html += "      method: 'POST',\n";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "      body: 'rxpin=' + rxPin + '&txpin=' + txPin + '&bootpin=' + bootPin + '&resetpin=' + resetPin + '&wakeuppin=' + wakeupPin + '&baudrate=' + baudrate\n";
  html += "    }).then(r => r.text()).then(msg => { saveAndReboot(msg); });\n";
  html += "  });\n";
  html += "}\n";
  html += "function resetModemSettings() {\n";
  html += "  showConfirm('Reset modem settings to defaults?', function() {\n";
  html += "    fetch('/api/reset-modem-settings', {method: 'POST'}).then(r => r.text()).then(msg => { saveAndReboot(msg); });\n";
  html += "  });\n";
  html += "}\n";
  // CW ID Settings
  html += "function saveCwidSettings() {\n";
  html += "  var enabled = document.getElementById('cwid-enabled').checked;\n";
  html += "  var interval = parseInt(document.getElementById('cwid-interval').value);\n";
  html += "  if (isNaN(interval) || interval < 1 || interval > 60) { showAlert('Interval must be 1-60 minutes'); return; }\n";
  html += "  showConfirm('Save CW ID settings?<br>Enabled: ' + (enabled ? 'Yes' : 'No') + '<br>Interval: ' + interval + ' min', function() {\n";
  html += "    fetch('/api/save-cwid-settings', {\n";
  html += "      method: 'POST',\n";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "      body: 'enabled=' + (enabled ? '1' : '0') + '&interval=' + interval\n";
  html += "    }).then(r => r.text()).then(msg => { saveAndReboot(msg); });\n";
  html += "  });\n";
  html += "}\n";
  html += "function resetCwidSettings() {\n";
  html += "  showConfirm('Reset CW ID settings to defaults?<br><br>Enabled: No<br>Interval: " + String(CWID_INTERVAL_MIN) + " min', function() {\n";
  html += "    fetch('/api/reset-cwid-settings', {method: 'POST'}).then(r => r.text()).then(msg => { saveAndReboot(msg); });\n";
  html += "  });\n";
  html += "}\n";
  html += "function testCwid() {\n";
  html += "  showConfirm('Transmit CW ID test now?<br><br>Callsign: " + userCallsign + "<br><br>Listen for Morse on your radio!', function() {\n";
  html += "    fetch('/api/test-cwid', {method: 'POST'}).then(r => r.text()).then(msg => { showAlert(msg); });\n";
  html += "  });\n";
  html += "}\n";
  html += "</script>";

  html += "</div>"; // Close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SETTINGS_MMDVM_H
