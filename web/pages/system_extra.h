/*
 * system_extra.h - Extra System Tools
 *
 * Provides interface for:
 * - Bootlogos package installer (LittleFS and SD card)
 */

#ifndef WEB_SYSTEM_EXTRA_H
#define WEB_SYSTEM_EXTRA_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

extern bool sdcardEnabled;

String getSystemExtraPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>System Extras</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("system-extra");

  html += "<div class='container'>";
  html += "<h1>System Extras</h1>";
  html += "<p>Extra tools and packages for the ESP32 MMDVM hotspot.</p>";
  html += "<div class='admin-grid'>";

  // Card: Bootlogos installer
  html += "<div class='card'>";
  html += "<h3>Bootlogos Package</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:12px;'>Download the official bootlogos package from GitHub and extract it to <code>/bootlogos</code> on the target filesystem. Existing files are overwritten. "
          "<a href='https://github.com/javastraat/esp32_mmdvm_hotspot/tree/main/images/bootlogos' target='_blank' style='color:#1976d2;'>Preview available logos &rarr;</a></p>";

  html += "<button class='btn btn-primary' id='bl-lfs-btn' onclick='blInstall(\"littlefs\")' style='margin-right:8px;margin-bottom:8px;'>Install to Flash (LittleFS)</button>";
  if (sdcardEnabled) {
    html += "<button class='btn btn-primary' id='bl-sd-btn' onclick='blInstall(\"sdcard\")' style='margin-bottom:8px;'>Install to SD Card</button>";
  }

  html += "<div id='bl-progress' style='display:none;margin-top:12px;'>";
  html += "  <div class='progress-bar'>";
  html += "    <div id='bl-progress-fill' class='progress-fill'></div>";
  html += "    <div id='bl-progress-text' class='progress-text'>0%</div>";
  html += "  </div>";
  html += "</div>";
  html += "<div id='bl-status' style='margin-top:10px;font-size:0.9em;color:#aaa;'></div>";
  html += "</div>";

  html += "</div>"; // Close admin-grid

  html += "<script>";
  html += "window.showModal = function(contentFn) { var overlay = document.createElement('div'); overlay.className = 'modal-overlay'; var box = document.createElement('div'); box.className = 'modal-box'; contentFn(box, function() { document.body.removeChild(overlay); }); overlay.appendChild(box); overlay.addEventListener('click', function(e) { if (e.target === overlay) document.body.removeChild(overlay); }); document.body.appendChild(overlay); return overlay; };";
  html += "window.showAlert = function(msg) { showModal(function(box, close) { box.innerHTML = '<h4>' + msg + '</h4>'; var btns = document.createElement('div'); btns.className = 'modal-buttons'; var ok = document.createElement('button'); ok.textContent = 'OK'; ok.className = 'btn btn-primary'; ok.onclick = close; btns.appendChild(ok); box.appendChild(btns); }); };";

  // Bootlogos installer JS
  html += "var blPollTimer = null;";
  html += "function blSetBusy(busy) {";
  html += "  var lfsBtn = document.getElementById('bl-lfs-btn');";
  html += "  if (lfsBtn) lfsBtn.disabled = busy;";
  html += "  var sdBtn = document.getElementById('bl-sd-btn');";
  html += "  if (sdBtn) sdBtn.disabled = busy;";
  html += "  document.getElementById('bl-progress').style.display = busy ? 'block' : 'none';";
  html += "}";
  html += "function blUpdateStatus(data) {";
  html += "  var fill = document.getElementById('bl-progress-fill');";
  html += "  var txt  = document.getElementById('bl-progress-text');";
  html += "  var stat = document.getElementById('bl-status');";
  html += "  if (fill) fill.style.width = data.progress + '%';";
  html += "  if (txt)  txt.textContent  = data.progress + '%';";
  html += "  if (stat) stat.textContent = data.status + (data.active && data.files > 0 ? ' (' + data.files + ' files)' : '');";
  html += "  if (!data.active) {";
  html += "    if (blPollTimer) { clearInterval(blPollTimer); blPollTimer = null; }";
  html += "    blSetBusy(false);";
  html += "    var isDone = data.status && data.status.indexOf('Done') === 0;";
  html += "    if (stat) stat.style.color = isDone ? '#2e7d32' : '#c62828';";
  html += "  }";
  html += "}";
  html += "function blPoll() {";
  html += "  fetch('/api/bootlogos/status').then(function(r){return r.json();}).then(blUpdateStatus).catch(function(){});";
  html += "}";
  html += "function blInstall(target) {";
  html += "  blSetBusy(true);";
  html += "  document.getElementById('bl-status').textContent = 'Starting...';";
  html += "  document.getElementById('bl-status').style.color = '#aaa';";
  html += "  document.getElementById('bl-progress-fill').style.width = '0%';";
  html += "  document.getElementById('bl-progress-text').textContent = '0%';";
  html += "  fetch('/api/bootlogos/install?target=' + target, {method:'POST'})";
  html += "    .then(function(r){return r.json();})";
  html += "    .then(function(d){";
  html += "      if (d.status === 'started') {";
  html += "        if (blPollTimer) clearInterval(blPollTimer);";
  html += "        blPollTimer = setInterval(blPoll, 1000);";
  html += "      } else {";
  html += "        blSetBusy(false);";
  html += "        document.getElementById('bl-status').textContent = d.message || d.status;";
  html += "        document.getElementById('bl-status').style.color = '#c62828';";
  html += "      }";
  html += "    })";
  html += "    .catch(function(){";
  html += "      blSetBusy(false);";
  html += "      document.getElementById('bl-status').textContent = 'Network error';";
  html += "      document.getElementById('bl-status').style.color = '#c62828';";
  html += "    });";
  html += "}";

  html += "</script>";

  html += "</div>"; // Close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SYSTEM_EXTRA_H
