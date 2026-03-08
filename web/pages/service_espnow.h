/*
 * service_espnow.h - ESP-NOW Configuration Page
 *
 * Three cards:
 *   1. Sender — enable sender, receiver MAC, debug log
 *   2. Receiver — enable receiver mode (future use)
 *   3. Modes — per-protocol forwarding toggles (DMR, POCSAG)
 */

#ifndef WEB_SERVICE_ESPNOW_H
#define WEB_SERVICE_ESPNOW_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

extern bool   espnowSenderEnabled;
extern bool   espnowReceiverEnabled;
extern String espnowReceiverMac;
extern bool   espnowDebug;
extern bool   espnowDmrEnabled;
extern bool   espnowPocsagEnabled;

String getServiceEspnowPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP-NOW Configuration</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("service-espnow");

  html += "<div class='container'>";
  html += "<h1>ESP-NOW Configuration</h1>";
  html += "<p>Bridge DMR and POCSAG frames to a second ESP32+modem over ESP-NOW (peer-to-peer WiFi, no router needed).</p>";
  html += "<div class='admin-grid'>";

  // ── Card 1: Sender ────────────────────────────────────────────────────────
  html += "<div class='card'>";
  html += "<h3>Sender</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>When enabled, this device forwards frames to the receiver over ESP-NOW. WiFi must be up for ESP-NOW to work.</p>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enable Sender:</span>";
  html += "<label class='switch'><input type='checkbox' id='en-sender'" + String(espnowSenderEnabled ? " checked" : "") + "><span class='slider'></span></label>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Receiver MAC:</span>";
  html += "<input type='text' id='recv-mac' value='" + espnowReceiverMac + "' placeholder='AA:BB:CC:DD:EE:FF' style='width:150px;font-family:monospace;padding-right:8px;' maxlength='17'>";
  html += "</div>";
  html += "<p style='font-size:0.82em;color:#888;margin-top:4px;'>MAC of the receiver device. Flash it with receiver firmware to print its MAC on boot.</p>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Debug Logging:</span>";
  html += "<label class='switch'><input type='checkbox' id='en-debug'" + String(espnowDebug ? " checked" : "") + "><span class='slider'></span></label>";
  html += "</div>";
  html += "<p style='font-size:0.82em;color:#888;margin-top:4px;'>Log No-ACK warnings. Disable unless debugging — generates ~1 log per DMR frame.</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveSender()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetSender()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // ── Card 2: Receiver ──────────────────────────────────────────────────────
  html += "<div class='card'>";
  html += "<h3>Receiver</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Future use. When this device acts as the remote node, enable receiver mode so it can accept incoming ESP-NOW frames and feed them to its local modem.</p>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enable Receiver:</span>";
  html += "<label class='switch'><input type='checkbox' id='en-receiver'" + String(espnowReceiverEnabled ? " checked" : "") + "><span class='slider'></span></label>";
  html += "</div>";
  html += "<p style='font-size:0.82em;color:#888;margin-top:4px;'>Receiver integration in firmware is not yet active. This flag is reserved for future use.</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveReceiver()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetReceiver()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // ── Card 3: Protocol Modes ────────────────────────────────────────────────
  html += "<div class='card'>";
  html += "<h3>Protocol Modes</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Choose which protocol frames are forwarded over ESP-NOW. Sender must be enabled above.</p>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Forward DMR:</span>";
  html += "<label class='switch'><input type='checkbox' id='en-dmr'" + String(espnowDmrEnabled ? " checked" : "") + "><span class='slider'></span></label>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Forward POCSAG:</span>";
  html += "<label class='switch'><input type='checkbox' id='en-pocsag'" + String(espnowPocsagEnabled ? " checked" : "") + "><span class='slider'></span></label>";
  html += "</div>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveModes()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetModes()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  html += "</div>"; // close admin-grid

  html += "<div class='info' style='margin-top:20px'>";
  html += "<strong>Note:</strong> Sender and Mode settings take effect at next boot (ESP-NOW is initialized once during startup). Receiver and Mode toggles are saved immediately to NVS.";
  html += "</div>";

  // ── JavaScript ────────────────────────────────────────────────────────────
  html += "<script>";

  html += "window.showModal=function(fn){";
  html += "var o=document.createElement('div');o.className='modal-overlay';";
  html += "var b=document.createElement('div');b.className='modal-box';";
  html += "fn(b,function(){document.body.removeChild(o);});";
  html += "o.appendChild(b);";
  html += "o.addEventListener('click',function(e){if(e.target===o)document.body.removeChild(o);});";
  html += "document.body.appendChild(o);return o;};";

  html += "window.showAlert=function(msg){";
  html += "showModal(function(b,close){";
  html += "b.innerHTML='<h4>'+msg+'</h4>';";
  html += "var d=document.createElement('div');d.className='modal-buttons';";
  html += "var ok=document.createElement('button');ok.textContent='OK';ok.className='btn btn-primary';ok.onclick=close;";
  html += "d.appendChild(ok);b.appendChild(d);});};";

  html += "window.showConfirm=function(msg,onYes){";
  html += "showModal(function(b,close){";
  html += "b.innerHTML='<h4>'+msg+'</h4>';";
  html += "var d=document.createElement('div');d.className='modal-buttons';";
  html += "var y=document.createElement('button');y.textContent='Yes';y.className='btn btn-success';";
  html += "y.onclick=function(){close();onYes();};";
  html += "var n=document.createElement('button');n.textContent='Cancel';n.className='btn btn-danger';n.onclick=close;";
  html += "d.appendChild(y);d.appendChild(n);b.appendChild(d);});};";

  // Sender
  html += "function saveSender(){";
  html += "var mac=document.getElementById('recv-mac').value.trim().toUpperCase();";
  html += "var re=/^([0-9A-F]{2}:){5}[0-9A-F]{2}$/;";
  html += "if(!re.test(mac)){showAlert('Invalid MAC address format. Use AA:BB:CC:DD:EE:FF');return;}";
  html += "var en=document.getElementById('en-sender').checked?'1':'0';";
  html += "var dbg=document.getElementById('en-debug').checked?'1':'0';";
  html += "showConfirm('Save sender settings?',function(){";
  html += "fetch('/api/save-espnow-sender',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},";
  html += "body:'sender='+en+'&mac='+encodeURIComponent(mac)+'&debug='+dbg})";
  html += ".then(r=>r.text()).then(msg=>{showAlert(msg);});});";
  html += "}";

  html += "function resetSender(){";
  html += "showConfirm('Reset sender settings to default?',function(){";
  html += "fetch('/api/reset-espnow-sender',{method:'POST'}).then(r=>r.text()).then(msg=>{showAlert(msg);location.reload();});});";
  html += "}";

  // Receiver
  html += "function saveReceiver(){";
  html += "var en=document.getElementById('en-receiver').checked?'1':'0';";
  html += "showConfirm('Save receiver setting?',function(){";
  html += "fetch('/api/save-espnow-receiver',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'receiver='+en})";
  html += ".then(r=>r.text()).then(msg=>{showAlert(msg);});});";
  html += "}";

  html += "function resetReceiver(){";
  html += "showConfirm('Reset receiver setting to default?',function(){";
  html += "fetch('/api/reset-espnow-receiver',{method:'POST'}).then(r=>r.text()).then(msg=>{showAlert(msg);location.reload();});});";
  html += "}";

  // Modes
  html += "function saveModes(){";
  html += "var dmr=document.getElementById('en-dmr').checked?'1':'0';";
  html += "var pocsag=document.getElementById('en-pocsag').checked?'1':'0';";
  html += "showConfirm('Save protocol mode settings?',function(){";
  html += "fetch('/api/save-espnow-modes',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},";
  html += "body:'dmr='+dmr+'&pocsag='+pocsag})";
  html += ".then(r=>r.text()).then(msg=>{showAlert(msg);});});";
  html += "}";

  html += "function resetModes(){";
  html += "showConfirm('Reset protocol modes to default?',function(){";
  html += "fetch('/api/reset-espnow-modes',{method:'POST'}).then(r=>r.text()).then(msg=>{showAlert(msg);location.reload();});});";
  html += "}";

  html += "</script>";

  html += "</div>"; // close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SERVICE_ESPNOW_H
