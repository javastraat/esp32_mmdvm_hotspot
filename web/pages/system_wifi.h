/*
 * WiFi Configuration Page (adapted from hotspot wifi_config.h)
 * WiFi network settings and management for 6 slots
 */

#ifndef WEB_SYSTEM_WIFI_H
#define WEB_SYSTEM_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

extern String wifiSlotLabel[6];
extern String wifiSlotSsid[6];
extern String wifiSlotPass[6];

extern String wifiApSsid;
extern String wifiApPassword;
extern uint8_t wifiApChannel;
extern uint8_t wifiMaxRetries;
extern bool softAPActive;

inline String getSystemWifiPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>WiFi Configuration</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("system-wifi");

  html += "<div class='container'>";
  html += "<h1>WiFi Configuration</h1>";
  html += "<p>Configure WiFi network connection and access point settings.</p>";
  html += "<div class='admin-grid'>";

  // Card 1: Current WiFi Status
  html += "<div class='card'>";
  html += "<h3>Current WiFi Status</h3>";
  if (WiFi.isConnected())
  {
    html += "<div class='status connected'>Status: Connected</div>";
    html += "<div class='metric'><span class='metric-label'>SSID:</span><span class='metric-value'>" + WiFi.SSID() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>IP Address:</span><span class='metric-value'>" + WiFi.localIP().toString() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Gateway:</span><span class='metric-value'>" + WiFi.gatewayIP().toString() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Subnet Mask:</span><span class='metric-value'>" + WiFi.subnetMask().toString() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>DNS Server:</span><span class='metric-value'>" + WiFi.dnsIP().toString() + "</span></div>";
    int rssi = WiFi.RSSI();
    String signalQuality = (rssi >= -50) ? "Excellent" : (rssi >= -60) ? "Good"
                                                     : (rssi >= -70)   ? "Fair"
                                                                       : "Weak";
    html += "<div class='metric'><span class='metric-label'>Signal Strength:</span><span class='metric-value'>" + String(rssi) + " dBm (" + signalQuality + ")</span></div>";
    html += "<div class='metric'><span class='metric-label'>Channel:</span><span class='metric-value'>" + String(WiFi.channel()) + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>MAC Address:</span><span class='metric-value'>" + WiFi.macAddress() + "</span></div>";
  }
  else if (softAPActive)
  {
    html += "<div class='status warning'>Status: Access Point Mode</div>";
    html += "<div class='metric'><span class='metric-label'>SSID:</span><span class='metric-value'>" + WiFi.softAPSSID() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>AP IP:</span><span class='metric-value'>" + WiFi.softAPIP().toString() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Clients:</span><span class='metric-value'>" + String(WiFi.softAPgetStationNum()) + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Channel:</span><span class='metric-value'>" + String(WiFi.channel()) + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>MAC Address:</span><span class='metric-value'>" + WiFi.softAPmacAddress() + "</span></div>";
  }
  else
  {
    html += "<div class='status disconnected'>Status: Disconnected</div>";
    html += "<div class='metric'><span class='metric-label'>MAC Address:</span><span class='metric-value'>" + WiFi.macAddress() + "</span></div>";
  }
  html += "</div>";

  // Combined Card: Slot Selector + Network Configuration
  html += "<div class='card'>";
  html += "<h3>Configure WiFi Networks</h3>";
  html += "<div class='info'>Configure up to 6 WiFi networks. The ESP32 will try these in order if the previous fails.</div>";
  html += "<form onsubmit=\"return false;\">";
  html += "<input type='hidden' id='currentSlot' value='0'>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Select Slot:</span>";
  html += "<select id='slotSelector' style='width: 120px; padding-right: 8px; margin-left: 8px;'>";
  for (int i = 0; i < 6; i++)
  {
    String escapedLabel = wifiSlotLabel[i];
    escapedLabel.replace("&", "&amp;");
    escapedLabel.replace("<", "&lt;");
    escapedLabel.replace(">", "&gt;");
    escapedLabel.replace("\"", "&quot;");
    String optText = "Slot " + String(i + 1) + ": " + escapedLabel;
    if (wifiSlotSsid[i].length() > 0)
    {
      String escapedSsid = wifiSlotSsid[i];
      escapedSsid.replace("&", "&amp;");
      escapedSsid.replace("<", "&lt;");
      escapedSsid.replace(">", "&gt;");
      escapedSsid.replace("\"", "&quot;");
      optText += " (" + escapedSsid + ")";
    }
    html += "<option value='" + String(i) + "'>" + optText + "</option>";
  }
  html += "</select>";
  html += "</div>";
  html += "<div style='margin-top:16px;'></div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Label:</span>";
  html += "<input type='text' id='labelInput' placeholder='Network label' maxlength='20' value='" + wifiSlotLabel[0] + "' style='box-sizing:border-box; width:120px; padding-right:8px; margin-left:8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>WiFi SSID:</span>";
  html += "<input type='text' id='ssidInput' placeholder='WiFi network name' value='" + wifiSlotSsid[0] + "' style='box-sizing:border-box; width:120px; padding-right:8px; margin-left:8px;' autocomplete='username'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>WiFi Password:</span>";
  html += "<input type='password' id='passwordInput' placeholder='WiFi password' value='" + wifiSlotPass[0] + "' style='box-sizing:border-box; width:120px; padding-right:28px; margin-left:8px;' autocomplete='current-password'>";
  html += "<span id='password-eye' style=\"position:absolute; right:8px; top:50%; transform:translateY(-50%); cursor:pointer;\" onclick=\"togglePasswordVisibility('passwordInput', this)\"><svg width=18 height=18 viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><path d='M1 12s4-7 11-7 11 7 11 7-4 7-11 7-11-7-11-7z'/><circle cx='12' cy='12' r='3'/></svg></span>";
  html += "</div>";
  html += "<div class='action-buttons-vertical' style='margin-top:10px;'>";
  html += "<button type='button' onclick='saveWifiSlot()' class='btn btn-success' style='width:100%;margin-bottom:8px;'>Save This Network</button>";
  html += "<button type='button' onclick='resetSlot()' class='btn btn-danger' style='width:100%;'>Reset Slot</button>";
  html += "</div>";
  html += "</form>";
  html += "</div>";

  // Card 3: Available Networks
  html += "<div class='card'>";
  html += "<h3>Available Networks</h3>";
  html += "<button class='btn btn-warning' onclick='scanNetworks()'>Scan for Networks</button>";
  html += "<div id='scan-results' class='scan-results' style='display:none;'>";
  html += "<div id='networks'>Scanning...</div>";
  html += "</div>";
  html += "</div>";

  // Card 4: WiFi Access Point
  html += "<div class='card'>";
  html += "<h3>WiFi Access Point</h3>";
  html += "<form onsubmit=\"return false;\">";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>AP SSID:</span>";
  html += "<input type='text' id='wifi-ap-ssid' value='" + wifiApSsid + "' maxlength='32' style='width: 120px; padding-right: 8px;' autocomplete='username'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>AP Password:</span>";
  html += "<input type='password' id='wifi-ap-password' value='" + wifiApPassword + "' minlength='8' maxlength='63' style='width: 104px; padding-right: 24px;' autocomplete='current-password'>";
  html += "<span id='wifi-ap-password-eye' style=\"position:absolute; right:8px; top:50%; transform:translateY(-50%); cursor:pointer;\" onclick=\"togglePasswordVisibility('wifi-ap-password', this)\"><svg width=18 height=18 viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><path d='M1 12s4-7 11-7 11 7 11 7-4 7-11 7-11-7-11-7z'/><circle cx='12' cy='12' r='3'/></svg></span>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>AP Channel:</span>";
  html += "<input type='text' id='wifi-ap-channel' value='" + String(wifiApChannel) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Max Retries:</span>";
  html += "<input type='text' id='wifi-max-retries' value='" + String(wifiMaxRetries) + "' style='width: 120px; padding-right: 8px;'>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>WiFi AP is used as fallback when connection fails. Password must be at least 8 characters.</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' type='button' onclick='saveWifiApSettings()'>Save &amp; Reboot</button>";
  html += "<button class='btn btn-danger' type='button' onclick='resetWifiApSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "</form>";
  html += "</div>";

  html += "</div>"; // close admin-grid

  // Save All & Reboot button
  html += "<div class='action-buttons-vertical' style='margin-top:20px;'>";
  html += "<button class='btn btn-success' onclick='saveAllWifiSettings()'>Save All &amp; Reboot</button>";
  html += "</div>";
  html += "<div class='info' style='margin-top:20px'>";
  html += "<strong>Note:</strong> WiFi slot changes are saved immediately without a reboot. Use <b>Save All &amp; Reboot</b> to save the current slot and AP settings together, then reboot.";
  html += "</div>";

  html += "<script>\n";
  html += "var currentSlot = 0;\n";
  html += "window.onload = function() {\n";
  html += "  function loadSlot(slot) {\n";
  html += "    fetch('/api/get-wifi-slot?slot=' + slot)\n";
  html += "      .then(function(r) { return r.json(); })\n";
  html += "      .then(function(data) {\n";
  html += "        document.getElementById('labelInput').value = data.label || '';\n";
  html += "        document.getElementById('ssidInput').value = data.ssid || '';\n";
  html += "        document.getElementById('passwordInput').value = data.password || '';\n";
  html += "        document.getElementById('currentSlot').value = slot;\n";
  html += "        currentSlot = slot;\n";
  html += "      })\n";
  html += "      .catch(function() {\n";
  html += "        document.getElementById('labelInput').value = '';\n";
  html += "        document.getElementById('ssidInput').value = '';\n";
  html += "        document.getElementById('passwordInput').value = '';\n";
  html += "      });\n";
  html += "  }\n";
  html += "  document.getElementById('slotSelector').addEventListener('change', function() {\n";
  html += "    loadSlot(parseInt(this.value));\n";
  html += "  });\n";
  html += "  window.loadSlot = loadSlot;\n";
  html += "  window.togglePasswordVisibility = function(id, icon) {\n";
  html += "    var input = document.getElementById(id);\n";
  html += "    if (input.type === 'password') {\n";
  html += "      input.type = 'text';\n";
  html += "      icon.innerHTML = \"<svg width=18 height=18 viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><path d='M1 12s4-7 11-7 11 7 11 7-4 7-11 7-11-7-11-7z'/><circle cx='12' cy='12' r='3'/></svg>\";\n";
  html += "    } else {\n";
  html += "      input.type = 'password';\n";
  html += "      icon.innerHTML = \"<svg width=18 height=18 viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><path d='M1 12s4-7 11-7 11 7 11 7-4 7-11 7-11-7-11-7z'/><circle cx='12' cy='12' r='3'/></svg>\";\n";
  html += "    }\n";
  html += "  };\n";
  html += "  window.renderNetworks = function(data) {\n";
  html += "    var h = '';\n";
  html += "    if (data.networks.length === 0) {\n";
  html += "      h = '<div class=\"network-item\">No networks found</div>';\n";
  html += "    } else {\n";
  html += "      data.networks.forEach(function(network, idx) {\n";
  html += "        var sq = '';\n";
  html += "        if (network.rssi >= -50) sq = 'Excellent';\n";
  html += "        else if (network.rssi >= -60) sq = 'Good';\n";
  html += "        else if (network.rssi >= -70) sq = 'Fair';\n";
  html += "        else sq = 'Weak';\n";
  html += "        var safeSsid = network.ssid.replace(/'/g, '');\n";
  html += "        h += '<div style=\"cursor:pointer;margin-bottom:8px;padding:8px;border:1px solid var(--border-color);border-radius:6px;\" onclick=\"promptAddNetwork(\\'' + safeSsid + '\\',' + network.rssi + ',\\'' + network.encryption + '\\')\">'\n";
  html += "          + '<div><strong>' + network.ssid + '</strong></div>'\n";
  html += "          + '<div style=\"font-size:0.85em;color:var(--text-muted);margin-top:4px;\">' + network.rssi + ' dBm (' + sq + ') &bull; ' + network.encryption + '</div>'\n";
  html += "        + '</div>';\n";
  html += "      });\n";
  html += "    }\n";
  html += "    return h;\n";
  html += "  };\n";
  html += "  window.showModal = function(contentFn) {\n";
  html += "    var overlay = document.createElement('div');\n";
  html += "    overlay.className = 'modal-overlay';\n";
  html += "    var box = document.createElement('div');\n";
  html += "    box.className = 'modal-box';\n";
  html += "    contentFn(box, function() { document.body.removeChild(overlay); });\n";
  html += "    overlay.appendChild(box);\n";
  html += "    overlay.addEventListener('click', function(e) { if (e.target === overlay) document.body.removeChild(overlay); });\n";
  html += "    document.body.appendChild(overlay);\n";
  html += "    return overlay;\n";
  html += "  };\n";
  html += "  window.showAlert = function(msg) {\n";
  html += "    showModal(function(box, close) {\n";
  html += "      box.innerHTML = '<h4>' + msg + '</h4>';\n";
  html += "      var btns = document.createElement('div');\n";
  html += "      btns.className = 'modal-buttons';\n";
  html += "      var ok = document.createElement('button');\n";
  html += "      ok.textContent = 'OK';\n";
  html += "      ok.className = 'btn btn-primary';\n";
  html += "      ok.onclick = close;\n";
  html += "      btns.appendChild(ok);\n";
  html += "      box.appendChild(btns);\n";
  html += "    });\n";
  html += "  };\n";
  html += "  window.showConfirm = function(msg, onYes) {\n";
  html += "    showModal(function(box, close) {\n";
  html += "      box.innerHTML = '<h4>' + msg + '</h4>';\n";
  html += "      var btns = document.createElement('div');\n";
  html += "      btns.className = 'modal-buttons';\n";
  html += "      var yes = document.createElement('button');\n";
  html += "      yes.textContent = 'Yes';\n";
  html += "      yes.className = 'btn btn-success';\n";
  html += "      yes.onclick = function() { close(); onYes(); };\n";
  html += "      var no = document.createElement('button');\n";
  html += "      no.textContent = 'Cancel';\n";
  html += "      no.className = 'btn btn-danger';\n";
  html += "      no.onclick = close;\n";
  html += "      btns.appendChild(yes);\n";
  html += "      btns.appendChild(no);\n";
  html += "      box.appendChild(btns);\n";
  html += "    });\n";
  html += "  };\n";
  html += "  window.promptAddNetwork = function(ssid, rssi, encryption) {\n";
  html += "    function showSlotPopup(ssid, password) {\n";
  html += "      showModal(function(box, close) {\n";
  html += "        box.innerHTML = '<h4>Add network <b>' + ssid + '</b></h4>';\n";
  html += "        var lbl = document.createElement('div');\n";
  html += "        lbl.textContent = 'Select slot:';\n";
  html += "        lbl.style.marginBottom = '8px';\n";
  html += "        box.appendChild(lbl);\n";
  html += "        var sel = document.createElement('select');\n";
  html += "        for (var i = 0; i < 6; i++) {\n";
  html += "          var label = document.getElementById('slotSelector').options[i].text;\n";
  html += "          sel.options.add(new Option(label, i));\n";
  html += "        }\n";
  html += "        box.appendChild(sel);\n";
  html += "        var btns = document.createElement('div');\n";
  html += "        btns.className = 'modal-buttons';\n";
  html += "        var okBtn = document.createElement('button');\n";
  html += "        okBtn.textContent = 'Save';\n";
  html += "        okBtn.className = 'btn btn-success';\n";
  html += "        okBtn.onclick = function() { close(); saveScannedNetworkToSlot(ssid, password, sel.value); };\n";
  html += "        var cancelBtn = document.createElement('button');\n";
  html += "        cancelBtn.textContent = 'Cancel';\n";
  html += "        cancelBtn.className = 'btn btn-danger';\n";
  html += "        cancelBtn.onclick = close;\n";
  html += "        btns.appendChild(okBtn);\n";
  html += "        btns.appendChild(cancelBtn);\n";
  html += "        box.appendChild(btns);\n";
  html += "      });\n";
  html += "    }\n";
  html += "    if (encryption === 'Open') { showSlotPopup(ssid, ''); return; }\n";
  html += "    showModal(function(box, close) {\n";
  html += "      box.innerHTML = '<h4>Password for <b>' + ssid + '</b></h4>';\n";
  html += "      var pwInput = document.createElement('input');\n";
  html += "      pwInput.type = 'password';\n";
  html += "      pwInput.placeholder = 'WiFi password';\n";
  html += "      box.appendChild(pwInput);\n";
  html += "      var btns = document.createElement('div');\n";
  html += "      btns.className = 'modal-buttons';\n";
  html += "      var okBtn = document.createElement('button');\n";
  html += "      okBtn.textContent = 'Next';\n";
  html += "      okBtn.className = 'btn btn-success';\n";
  html += "      okBtn.onclick = function() { var pw = pwInput.value; close(); showSlotPopup(ssid, pw); };\n";
  html += "      var cancelBtn = document.createElement('button');\n";
  html += "      cancelBtn.textContent = 'Cancel';\n";
  html += "      cancelBtn.className = 'btn btn-danger';\n";
  html += "      cancelBtn.onclick = close;\n";
  html += "      btns.appendChild(okBtn);\n";
  html += "      btns.appendChild(cancelBtn);\n";
  html += "      box.appendChild(btns);\n";
  html += "      setTimeout(function() { pwInput.focus(); }, 100);\n";
  html += "    });\n";
  html += "  };\n";
  html += "  window.saveScannedNetworkToSlot = function(ssid, password, slot) {\n";
  html += "    var label = ssid;\n";
  html += "    fetch('/api/save-wifi-slot', {\n";
  html += "      method: 'POST',\n";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "      body: 'slot=' + slot + '&label=' + encodeURIComponent(label) + '&ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password)\n";
  html += "    }).then(function(r) { return r.text(); }).then(function(msg) {\n";
  html += "      showAlert(msg);\n";
  html += "      loadSlot(slot);\n";
  html += "      var sel = document.getElementById('slotSelector');\n";
  html += "      sel.options[slot].text = 'Slot ' + (parseInt(slot) + 1) + ': ' + label + ' (' + ssid + ')';\n";
  html += "    }).catch(function(err) { showAlert('Error: ' + err); });\n";
  html += "  };\n";
  html += "  window.scanNetworks = function() { document.getElementById('scan-results').style.display = 'block'; document.getElementById('networks').innerHTML = 'Scanning...'; fetch('/api/wifiscan').then(r => r.json()).then(data => { document.getElementById('networks').innerHTML = renderNetworks(data); }).catch(e => { document.getElementById('networks').innerHTML = '<div class=\"network-item\">Scan failed</div>'; }); };\n";
  html += "  window.selectNetwork = function(ssid) { document.getElementById('ssidInput').value = ssid; document.getElementById('passwordInput').focus(); };\n";
  html += "  window.saveWifiSlot = function() {\n";
  html += "    var slot = document.getElementById('currentSlot').value;\n";
  html += "    var label = document.getElementById('labelInput').value;\n";
  html += "    var ssid = document.getElementById('ssidInput').value;\n";
  html += "    var password = document.getElementById('passwordInput').value;\n";
  html += "    fetch('/api/save-wifi-slot', {\n";
  html += "      method: 'POST',\n";
  html += "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "      body: 'slot=' + slot + '&label=' + encodeURIComponent(label) + '&ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password)\n";
  html += "    }).then(function(r) { return r.text(); }).then(function(msg) {\n";
  html += "      showAlert(msg);\n";
  html += "      fetch('/api/get-wifi-slots').then(function(r) { return r.json(); }).then(function(data) {\n";
  html += "        var sel = document.getElementById('slotSelector');\n";
  html += "        for (var i = 0; i < 6; i++) {\n";
  html += "          var s = data.slots[i];\n";
  html += "          var optText = 'Slot ' + (i+1) + ': ' + (s.label ? s.label : '-');\n";
  html += "          if (s.ssid) optText += ' (' + s.ssid + ')';\n";
  html += "          sel.options[i].text = optText;\n";
  html += "        }\n";
  html += "      });\n";
  html += "    }).catch(function(err) { showAlert('Error: ' + err); });\n";
  html += "  };\n";
  html += "  window.resetSlot = function() {\n";
  html += "    showConfirm('Reset this slot to its default values from config?', function() {\n";
  html += "      fetch('/api/reset-wifi-slot', {\n";
  html += "        method: 'POST',\n";
  html += "        headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "        body: 'slot=' + currentSlot\n";
  html += "      }).then(function(r) { return r.json(); }).then(function(data) {\n";
  html += "        document.getElementById('labelInput').value = data.label || '';\n";
  html += "        document.getElementById('ssidInput').value = data.ssid || '';\n";
  html += "        document.getElementById('passwordInput').value = data.password || '';\n";
  html += "        var sel = document.getElementById('slotSelector');\n";
  html += "        var optText = 'Slot ' + (currentSlot + 1) + ': ' + (data.label || '');\n";
  html += "        if (data.ssid) optText += ' (' + data.ssid + ')';\n";
  html += "        sel.options[currentSlot].text = optText;\n";
  html += "        showAlert('Slot reset to default');\n";
  html += "      }).catch(function(err) { showAlert('Error: ' + err); });\n";
  html += "    });\n";
  html += "  };\n";
  // WiFi AP Settings
  html += "  window.saveWifiApSettings = function() {\n";
  html += "    var ssid = document.getElementById('wifi-ap-ssid').value;\n";
  html += "    var password = document.getElementById('wifi-ap-password').value;\n";
  html += "    var channel = document.getElementById('wifi-ap-channel').value;\n";
  html += "    var retries = document.getElementById('wifi-max-retries').value;\n";
  html += "    if (!ssid || ssid.length < 3) { showAlert('WiFi AP SSID must be at least 3 characters'); return; }\n";
  html += "    if (!password || password.length < 8) { showAlert('WiFi AP password must be at least 8 characters'); return; }\n";
  html += "    if (channel < 1 || channel > 13) { showAlert('WiFi AP channel must be 1-13'); return; }\n";
  html += "    if (retries < 1 || retries > 20) { showAlert('Max retries must be 1-20'); return; }\n";
  html += "    showConfirm('Save WiFi AP settings and reboot?', function() {\n";
  html += "      fetch('/api/save-wifi-ap-settings', {\n";
  html += "        method: 'POST',\n";
  html += "        headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "        body: 'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password) + '&channel=' + channel + '&retries=' + retries\n";
  html += "      }).then(r => r.text()).then(msg => {\n";
  html += "        showAlert(msg + '<br><br>The device will now reboot.');\n";
  html += "        fetch('/api/reboot', {method: 'POST'});\n";
  html += "        document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';\n";
  html += "        setTimeout(function() { location.reload(); }, 10000);\n";
  html += "      });\n";
  html += "    });\n";
  html += "  };\n";

  html += "  window.resetWifiApSettings = function() {\n";
  html += "    showConfirm('Reset WiFi AP settings to default and reboot?', function() {\n";
  html += "      fetch('/api/reset-wifi-ap-settings', {method: 'POST'}).then(r => r.text()).then(msg => {\n";
  html += "        showAlert(msg + '<br><br>The device will now reboot.');\n";
  html += "        fetch('/api/reboot', {method: 'POST'});\n";
  html += "        document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';\n";
  html += "        setTimeout(function() { location.reload(); }, 10000);\n";
  html += "      });\n";
  html += "    });\n";
  html += "  };\n";

  // Save All & Reboot: current slot + AP settings
  html += "  window.saveAllWifiSettings = function() {\n";
  html += "    var ssid = document.getElementById('wifi-ap-ssid').value;\n";
  html += "    var password = document.getElementById('wifi-ap-password').value;\n";
  html += "    var channel = document.getElementById('wifi-ap-channel').value;\n";
  html += "    var retries = document.getElementById('wifi-max-retries').value;\n";
  html += "    if (!ssid || ssid.length < 3) { showAlert('WiFi AP SSID must be at least 3 characters'); return; }\n";
  html += "    if (!password || password.length < 8) { showAlert('WiFi AP password must be at least 8 characters'); return; }\n";
  html += "    if (channel < 1 || channel > 13) { showAlert('WiFi AP channel must be 1-13'); return; }\n";
  html += "    if (retries < 1 || retries > 20) { showAlert('Max retries must be 1-20'); return; }\n";
  html += "    showConfirm('Save current WiFi slot and AP settings, then reboot?', function() {\n";
  html += "      var slot = document.getElementById('currentSlot').value;\n";
  html += "      var label = document.getElementById('labelInput').value;\n";
  html += "      var slotSsid = document.getElementById('ssidInput').value;\n";
  html += "      var slotPass = document.getElementById('passwordInput').value;\n";
  html += "      var post = function(url, body) { return fetch(url, {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:body}); };\n";
  html += "      post('/api/save-wifi-slot', 'slot=' + slot + '&label=' + encodeURIComponent(label) + '&ssid=' + encodeURIComponent(slotSsid) + '&password=' + encodeURIComponent(slotPass))\n";
  html += "      .then(function() { return post('/api/save-wifi-ap-settings', 'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password) + '&channel=' + channel + '&retries=' + retries); })\n";
  html += "      .then(function() {\n";
  html += "        showAlert('All WiFi settings saved.<br><br>The device will now reboot.');\n";
  html += "        fetch('/api/reboot', {method: 'POST'});\n";
  html += "        document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';\n";
  html += "        setTimeout(function() { location.reload(); }, 10000);\n";
  html += "      }).catch(function(err) { showAlert('Error saving settings: ' + err); });\n";
  html += "    });\n";
  html += "  };\n";

  html += "};\n";
  html += "</script>";
  html += "</div>"; // Close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SYSTEM_WIFI_H
