/*
 * Mode Selection Page
 * Enable/disable MMDVM protocol modes (DMR, D-Star, YSF, P25, NXDN, POCSAG)
 */

#ifndef WEB_MODE_SELECT_H
#define WEB_MODE_SELECT_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

// External references to runtime settings
extern bool modeDmrEnabled;
extern bool modeDstarEnabled;
extern bool modeYsfEnabled;
extern bool modeP25Enabled;
extern bool modeNxdnEnabled;
extern bool modePocsagEnabled;

String getModeSelectPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Mode Selection</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("mode-select");

  html += "<div class='container'>";
  html += "<h1>Mode Selection</h1>";
  html += "<p>Enable or disable MMDVM protocol modes. Changes require a reboot to take effect.</p>";

  html += "<div class='admin-grid'>";

  // DMR Card
  html += "<div class='card'>";
  html += "<h3>DMR</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enabled:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='mode-dmr'" + String(modeDmrEnabled ? " checked" : "") + ">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<p>Digital Mobile Radio - Tier II digital protocol</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='/mode-dmr' class='btn btn-primary'>Configure</a>";
  html += "</div>";
  html += "</div>";

  // D-Star Card
  html += "<div class='card'>";
  html += "<h3>D-Star</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enabled:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='mode-dstar'" + String(modeDstarEnabled ? " checked" : "") + " disabled>";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<p>Digital Smart Technologies for Amateur Radio</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='/mode-dstar' class='btn btn-primary'>Configure</a>";
  html += "</div>";
  html += "</div>";

  // YSF Card
  html += "<div class='card'>";
  html += "<h3>YSF/Fusion</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enabled:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='mode-ysf'" + String(modeYsfEnabled ? " checked" : "") + " disabled>";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<p>Yaesu System Fusion (C4FM) digital mode</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='/mode-ysf' class='btn btn-primary'>Configure</a>";
  html += "</div>";
  html += "</div>";

  // P25 Card
  html += "<div class='card'>";
  html += "<h3>P25</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enabled:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='mode-p25'" + String(modeP25Enabled ? " checked" : "") + " disabled>";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<p>Project 25 (APCO-25) public safety standard</p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='/mode-p25' class='btn btn-primary'>Configure</a>";
  html += "</div>";
  html += "</div>";

  // NXDN Card
  html += "<div class='card'>";
  html += "<h3>NXDN</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enabled:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='mode-nxdn'" + String(modeNxdnEnabled ? " checked" : "") + " disabled>";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<p>Next Generation Digital Narrowband  </p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='/mode-nxdn' class='btn btn-primary'>Configure</a>";
  html += "</div>";
  html += "</div>";

  // POCSAG Card
  html += "<div class='card'>";
  html += "<h3>POCSAG</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enabled:</span>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='mode-pocsag'" + String(modePocsagEnabled ? " checked" : "") + ">"; //+ " disabled>"
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "</div>";
  html += "<p>Paging system protocol (unidirectional) </p>";
  html += "<div class='action-buttons-vertical'>";
  html += "<a href='/mode-pocsag' class='btn btn-primary'>Configure</a>";
  html += "</div>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  // Save & Reboot button
  html += "<div class='action-buttons-vertical' style='margin-top:20px;'>";
  html += "<button class='btn btn-success' onclick='saveModeSettings()'>Save & Reboot</button>";
  html += "</div>";

  html += "<div class='info' style='margin-top:20px'>";
  html += "<strong>Note:</strong> After changing mode settings, click Save & Reboot for changes to take effect.";
  html += "</div>";

  // JavaScript for save functionality
  html += "<script>\n";
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
  html += "function saveModeSettings() {\n";
  html += "  var modes = [\n";
  html += "    {name: 'dmr', el: document.getElementById('mode-dmr')},\n";
  html += "    {name: 'dstar', el: document.getElementById('mode-dstar')},\n";
  html += "    {name: 'ysf', el: document.getElementById('mode-ysf')},\n";
  html += "    {name: 'p25', el: document.getElementById('mode-p25')},\n";
  html += "    {name: 'nxdn', el: document.getElementById('mode-nxdn')},\n";
  html += "    {name: 'pocsag', el: document.getElementById('mode-pocsag')}\n";
  html += "  ];\n";
  html += "  showConfirm('Save mode settings and reboot?', function() {\n";
  html += "    var promises = modes.filter(function(m) { return !m.el.disabled; }).map(function(m) {\n";
  html += "      return fetch('/api/mode-toggle', {\n";
  html += "        method: 'POST',\n";
  html += "        headers: {'Content-Type': 'application/x-www-form-urlencoded'},\n";
  html += "        body: 'mode=' + m.name + '&enable=' + m.el.checked\n";
  html += "      });\n";
  html += "    });\n";
  html += "    Promise.all(promises).then(function() {\n";
  html += "      showAlert('Mode settings saved. The device will now reboot.', function() {\n";
  html += "        fetch('/api/reboot', {method: 'POST'});\n";
  html += "        document.body.innerHTML = '<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';\n";
  html += "        setTimeout(function() { location.reload(); }, 10000);\n";
  html += "      });\n";
  html += "    });\n";
  html += "  });\n";
  html += "}\n";
  html += "</script>";

  html += "</div>"; // Close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_MODE_SELECT_H
