/*
 * service_espnow.h - ESP-NOW / UniversalMesh Configuration Page
 *
 * Three cards:
 *   1. Sender       — enable coordinator mode, debug log
 *   2. Mesh Status  — live coordinator/node status dots (polls /api/espnow-peer-status)
 *   3. Modes        — per-protocol forwarding toggles (DMR, POCSAG)
 *
 * No MAC address inputs — UniversalMesh auto-discovers peers via PING/PONG.
 * Receiver mode is automatic: selecting ESP-NOW as the DMR server source
 * (in DMR settings) makes this device act as a node — no separate flag needed.
 */

#ifndef WEB_SERVICE_ESPNOW_H
#define WEB_SERVICE_ESPNOW_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

extern bool   espnowSenderEnabled;
extern bool   espnowDebug;
extern bool   espnowDmrEnabled;
extern bool   espnowPocsagEnabled;

String getServiceEspnowPageHTML()
{
  String html;
  html.reserve(28000);
  html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP-NOW Configuration</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("service-espnow");

  html += "<div class='container'>";
  html += "<h1>ESP-NOW / UniversalMesh</h1>";
  html += "<p>Bridge DMR and POCSAG frames to nearby ESP32 nodes over ESP-NOW. ";
  html += "Nodes are discovered automatically via mesh PING/PONG — no MAC addresses to configure.</p>";
  html += "<div class='admin-grid'>";

  // ── Card 1: Sender / Coordinator ─────────────────────────────────────────
  html += "<div class='card'>";
  html += "<h3>Sender</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>When enabled, this device acts as the mesh <strong>coordinator</strong> and forwards frames to all discovered nodes. WiFi must be up.</p>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enable Sender:</span>";
  html += "<label class='switch'><input type='checkbox' id='en-sender'" + String(espnowSenderEnabled ? " checked" : "") + " onchange='syncSender()'><span class='slider'></span></label>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Debug Logging:</span>";
  html += "<label class='switch'><input type='checkbox' id='en-debug'" + String(espnowDebug ? " checked" : "") + "><span class='slider'></span></label>";
  html += "</div>";
  html += "<p style='font-size:0.82em;color:#888;margin-top:4px;'>Logs node announce messages. Disable unless debugging.</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveSender()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetSender()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // ── Card 2: Mesh Status ───────────────────────────────────────────────────
  html += "<div class='card'>";
  html += "<h3>Mesh Status</h3>";
  if (espnowSenderEnabled) {
    html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Coordinator mode: nodes that have announced themselves. Green = seen within 2 min.</p>";
  } else {
    html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Node mode: coordinator connection. Green = coordinator seen within 2 min. Orange = searching.</p>";
  }
  for (int i = 0; i < 6; i++) {
    html += "<div class='metric'>";
    html += "<span id='dot-" + String(i) + "' title='Loading...' style='display:inline-block;width:10px;height:10px;border-radius:50%;background:#ccc;margin-right:8px;vertical-align:middle;cursor:default;'></span>";
    html += "<span id='mac-label-" + String(i) + "' style='font-family:monospace;font-size:0.88em;color:#555;'>&mdash;</span>";
    html += "</div>";
  }
  html += "<p style='font-size:0.8em;color:#aaa;margin-top:8px;'>Refreshes every 5 s.</p>";
  html += "</div>";

  // ── Card 3: Protocol Modes ────────────────────────────────────────────────
  html += "<div class='card'>";
  html += "<h3>Protocol Modes</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Choose which protocol frames are forwarded. Sender must be enabled.</p>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Forward DMR:</span>";
  html += "<label class='switch'><input type='checkbox' id='en-dmr'" + String(espnowDmrEnabled ? " checked" : "") + " onchange='syncModes()'><span class='slider'></span></label>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Forward POCSAG:</span>";
  html += "<label class='switch'><input type='checkbox' id='en-pocsag'" + String(espnowPocsagEnabled ? " checked" : "") + " onchange='syncModes()'><span class='slider'></span></label>";
  html += "</div>";
  html += "<p style='font-size:0.82em;color:#888;margin-top:4px;'>Note: POCSAG messages over 58 chars are truncated by the 64-byte mesh payload limit.</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveModes()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetModes()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  html += "</div>"; // close admin-grid

  html += "<div class='info' style='margin-top:20px'>";
  html += "<strong>Note:</strong> Sender and Mode settings take effect at next boot. ";
  html += "To use this device as a receiver node, select <em>ESP-NOW</em> as the DMR server source in DMR settings — ";
  html += "it will automatically find the coordinator via mesh PING.";
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
  html += "var en=document.getElementById('en-sender').checked?'1':'0';";
  html += "var dbg=document.getElementById('en-debug').checked?'1':'0';";
  html += "showConfirm('Save sender settings?',function(){";
  html += "fetch('/api/save-espnow-sender',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},";
  html += "body:'sender='+en+'&debug='+dbg})";
  html += ".then(r=>r.text()).then(msg=>{showAlert(msg);});});";
  html += "}";

  html += "function resetSender(){";
  html += "showConfirm('Reset sender settings to default?',function(){";
  html += "fetch('/api/reset-espnow-sender',{method:'POST'}).then(r=>r.text()).then(msg=>{showAlert(msg);location.reload();});});";
  html += "}";

  html += "function syncSender(){";
  html += "  var on=document.getElementById('en-sender').checked;";
  html += "  var lock=on?'':'0.4';var pe=on?'':'none';";
  html += "  var dbg=document.getElementById('en-debug');";
  html += "  dbg.disabled=!on;dbg.closest('label').style.opacity=lock;dbg.closest('label').style.pointerEvents=pe;";
  html += "  var dmr=document.getElementById('en-dmr');var poc=document.getElementById('en-pocsag');";
  html += "  dmr.disabled=!on;dmr.closest('label').style.opacity=lock;dmr.closest('label').style.pointerEvents=pe;";
  html += "  poc.disabled=!on;poc.closest('label').style.opacity=lock;poc.closest('label').style.pointerEvents=pe;";
  html += "}";
  html += "syncSender();";

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

  html += "function syncModes(){";
  html += "  var d=document.getElementById('en-dmr');";
  html += "  var p=document.getElementById('en-pocsag');";
  html += "  if(d.checked){p.disabled=true;p.closest('label').style.opacity='0.4';p.closest('label').style.pointerEvents='none';}";
  html += "  else{p.disabled=false;p.closest('label').style.opacity='';p.closest('label').style.pointerEvents='';}";
  html += "  if(p.checked){d.disabled=true;d.closest('label').style.opacity='0.4';d.closest('label').style.pointerEvents='none';}";
  html += "  else{d.disabled=false;d.closest('label').style.opacity='';d.closest('label').style.pointerEvents='';}";
  html += "}";
  html += "syncModes();";

  // Mesh status dots — polls /api/espnow-peer-status every 5 s
  html += "var DOT_CFG={";
  html += "  ok:{color:'#4CAF50',tip:'Active — seen within 2 minutes'},";
  html += "  idle:{color:'#FF9800',tip:'Not seen recently or still searching'},";
  html += "  fail:{color:'#f44336',tip:'Connection lost'},";
  html += "  none:{color:'#ccc',tip:'No peer'}";
  html += "};";
  html += "function updateDots(){";
  html += "fetch('/api/espnow-peer-status').then(function(r){return r.json();}).then(function(data){";
  html += "  for(var i=0;i<6;i++){";
  html += "    var dot=document.getElementById('dot-'+i);";
  html += "    var lbl=document.getElementById('mac-label-'+i);";
  html += "    if(!dot||!lbl)continue;";
  html += "    var entry=data[i]||{};";
  html += "    var s=entry.status||'none';";
  html += "    var cfg=DOT_CFG[s]||DOT_CFG.none;";
  html += "    dot.style.background=cfg.color;dot.title=cfg.tip;";
  html += "    lbl.textContent=(entry.mac&&entry.mac!=='')?entry.mac:'\u2014';";
  html += "  }";
  html += "}).catch(function(){});";
  html += "}";
  html += "updateDots();setInterval(updateDots,5000);";

  html += "</script>";

  html += "</div>"; // close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SERVICE_ESPNOW_H
