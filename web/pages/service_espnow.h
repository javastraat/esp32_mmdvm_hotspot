/*
 * service_espnow.h - ESP-NOW Configuration Page
 *
 * Three cards:
 *   1. Sender       — enable sender, debug log
 *   2. Receiver MACs — up to 6 individual MAC inputs (stored comma-delimited)
 *   3. Modes        — per-protocol forwarding toggles (DMR, POCSAG)
 *
 * Receiver mode is automatic: selecting ESP-NOW as the DMR server source
 * (in DMR settings) makes this device act as a receiver — no separate flag needed.
 */

#ifndef WEB_SERVICE_ESPNOW_H
#define WEB_SERVICE_ESPNOW_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

extern bool   espnowSenderEnabled;
extern String espnowReceiverMac;
extern bool   espnowDebug;
extern bool   espnowDmrEnabled;
extern bool   espnowPocsagEnabled;

String getServiceEspnowPageHTML()
{
  String html;
  html.reserve(35000);
  html = "<!DOCTYPE html><html lang='en'><head>";
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
  html += "<label class='switch'><input type='checkbox' id='en-sender'" + String(espnowSenderEnabled ? " checked" : "") + " onchange='syncSender()'><span class='slider'></span></label>";
  html += "</div>";
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

  // ── Card 2: Receiver MACs ─────────────────────────────────────────────────
  // Split stored comma-delimited string into up to 6 slots for individual inputs
  String macSlots[6] = {"","","","","",""};
  {
    int idx = 0;
    int start = 0;
    for (int i = 0; i <= (int)espnowReceiverMac.length() && idx < 6; i++) {
      if (i == (int)espnowReceiverMac.length() || espnowReceiverMac[i] == ',') {
        String part = espnowReceiverMac.substring(start, i);
        part.trim();
        if (part.length() > 0) macSlots[idx++] = part;
        start = i + 1;
      }
    }
  }
  html += "<div class='card'>";
  html += "<h3>Receiver MACs</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Up to 6 receiver MAC addresses. Leave unused slots empty. Flash each receiver to print its MAC on boot.</p>";
  for (int i = 0; i < 6; i++) {
    html += "<div class='metric'>";
    html += "<span class='metric-label'>MAC " + String(i + 1) + ":</span>";
    html += "<input type='text' id='mac-" + String(i) + "' value='" + macSlots[i] + "' placeholder='AA:BB:CC:DD:EE:FF' style='font-family:monospace;width:160px;' maxlength='17'>";
    html += "<span id='dot-" + String(i) + "' title='No peer configured' style='display:inline-block;width:10px;height:10px;border-radius:50%;background:#ccc;margin-left:8px;vertical-align:middle;cursor:default;'></span>";
    html += "</div>";
  }
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveSender()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetSender()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // ── Card 3: Discover Receivers ────────────────────────────────────────────
  html += "<div class='card'>";
  html += "<h3>Discover Receivers</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Scan the local network for nearby devices running this firmware. Click <em>Use</em> to fill the next empty MAC slot above.</p>";
  html += "<button class='btn btn-primary' id='discover-btn' onclick='runDiscover()'>Scan</button>";
  html += "<div id='discover-list' style='margin-top:12px;'></div>";
  html += "</div>";

  // ── Card 4: Protocol Modes ────────────────────────────────────────────────
  html += "<div class='card'>";
  html += "<h3>Protocol Modes</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Choose which protocol frames are forwarded over ESP-NOW. Sender must be enabled above.</p>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Forward DMR:</span>";
  html += "<label class='switch'><input type='checkbox' id='en-dmr'" + String(espnowDmrEnabled ? " checked" : "") + " onchange='syncModes()'><span class='slider'></span></label>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Forward POCSAG:</span>";
  html += "<label class='switch'><input type='checkbox' id='en-pocsag'" + String(espnowPocsagEnabled ? " checked" : "") + " onchange='syncModes()'><span class='slider'></span></label>";
  html += "</div>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveModes()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetModes()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  html += "</div>"; // close admin-grid

  html += "<div class='info' style='margin-top:20px'>";
  html += "<strong>Note:</strong> Sender and Mode settings take effect at next boot (ESP-NOW is initialized once during startup). ";
  html += "To use this device as a receiver, select <em>ESP-NOW</em> as the DMR server source in DMR settings.";
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
  html += "function getMacs(){";
  html += "var single=/^([0-9A-F]{2}:){5}[0-9A-F]{2}$/;";
  html += "var out=[];";
  html += "for(var i=0;i<6;i++){";
  html += "  var v=document.getElementById('mac-'+i).value.trim().toUpperCase();";
  html += "  if(v==='')continue;";
  html += "  if(!single.test(v)){showAlert('Invalid MAC '+(i+1)+': use AA:BB:CC:DD:EE:FF');return null;}";
  html += "  out.push(v);";
  html += "}";
  html += "return out.join(',');";
  html += "}";

  html += "function saveSender(){";
  html += "var mac=getMacs();if(mac===null)return;";
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

  html += "function syncSender(){";
  html += "  var on=document.getElementById('en-sender').checked;";
  html += "  var lock=on?'':'0.4';var pe=on?'':'none';";
  html += "  for(var i=0;i<6;i++){var m=document.getElementById('mac-'+i);m.disabled=!on;m.style.opacity=lock;}";
  html += "  var dbg=document.getElementById('en-debug');var dbgl=dbg.closest('label');";
  html += "  dbg.disabled=!on;dbgl.style.opacity=lock;dbgl.style.pointerEvents=pe;";
  html += "  var dmr=document.getElementById('en-dmr');var dmrl=dmr.closest('label');";
  html += "  var poc=document.getElementById('en-pocsag');var pocl=poc.closest('label');";
  html += "  dmr.disabled=!on;dmrl.style.opacity=lock;dmrl.style.pointerEvents=pe;";
  html += "  poc.disabled=!on;pocl.style.opacity=lock;pocl.style.pointerEvents=pe;";
  html += "}";
  html += "syncSender();";
  html += "function syncModes(){";
  html += "  var d=document.getElementById('en-dmr');";
  html += "  var p=document.getElementById('en-pocsag');";
  html += "  var dl=d.closest('label');var pl=p.closest('label');";
  html += "  if(d.checked){p.disabled=true;pl.style.opacity='0.4';pl.style.pointerEvents='none';}";
  html += "  else{p.disabled=false;pl.style.opacity='';pl.style.pointerEvents='';}";
  html += "  if(p.checked){d.disabled=true;dl.style.opacity='0.4';dl.style.pointerEvents='none';}";
  html += "  else{d.disabled=false;dl.style.opacity='';dl.style.pointerEvents='';}";
  html += "}";
  html += "syncModes();";

  // Discover
  html += "function useDiscoveredMac(mac){";
  html += "for(var i=0;i<6;i++){var el=document.getElementById('mac-'+i);if(el&&el.value.trim()===''){el.value=mac;return;}}";
  html += "showAlert('All 6 MAC slots are already filled.');";
  html += "}";

  html += "function runDiscover(){";
  html += "var btn=document.getElementById('discover-btn');";
  html += "var list=document.getElementById('discover-list');";
  html += "btn.disabled=true;btn.textContent='Scanning...';";
  html += "list.innerHTML='<p style=\"color:#888;font-size:0.85em;\">Scanning\u2026</p>';";
  html += "fetch('/api/espnow-discover').then(function(r){return r.json();}).then(function(data){";
  html += "btn.disabled=false;btn.textContent='Scan';";
  html += "if(!data.length){list.innerHTML='<p style=\"color:#888;font-size:0.85em;\">No devices found.</p>';return;}";
  html += "var h='';for(var i=0;i<data.length;i++){";
  html += "var d=data[i];var modes=[];";
  html += "if(d.dmr_relay)modes.push('DMR');if(d.pocsag_relay)modes.push('POCSAG');";
  html += "var ms=modes.length?modes.join(', '):'SERVER';";
  html += "h+='<div class=\"metric\" style=\"flex-wrap:wrap;gap:6px;align-items:center;\">';";
  html += "h+='<span style=\"font-family:monospace;font-size:0.9em;\">'+d.mac+'</span>';";
  html += "h+='<span style=\"color:#555;font-size:0.85em;\">'+d.name+'</span>';";
  html += "h+='<span style=\"color:#888;font-size:0.8em;\">'+ms+'</span>';";
  html += "h+='<button class=\"btn btn-primary\" style=\"padding:2px 10px;font-size:0.8em;\" onclick=\"useDiscoveredMac(\\'' + d.mac + '\\')\">Use</button>';";
  html += "h+='</div>';}";
  html += "list.innerHTML=h;";
  html += "}).catch(function(){btn.disabled=false;btn.textContent='Scan';";
  html += "list.innerHTML='<p style=\"color:#f44336;font-size:0.85em;\">Scan failed.</p>';});";
  html += "}";

  // Peer status dots — polls /api/espnow-peer-status every 5 s
  html += "var DOT_CFG={ok:{color:'#4CAF50',tip:'Last frame acknowledged'},fail:{color:'#f44336',tip:'No ACK received — peer may be out of range'},idle:{color:'#FF9800',tip:'No traffic in the last 121 s'},none:{color:'#ccc',tip:'No peer configured'}};";
  html += "function updateDots(){";
  html += "fetch('/api/espnow-peer-status').then(function(r){return r.json();}).then(function(data){";
  html += "  for(var i=0;i<6;i++){";
  html += "    var d=document.getElementById('dot-'+i);if(!d)continue;";
  html += "    var s=(data[i]&&data[i].status)?data[i].status:'none';";
  html += "    var cfg=DOT_CFG[s]||DOT_CFG.none;";
  html += "    d.style.background=cfg.color;d.title=cfg.tip;";
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
