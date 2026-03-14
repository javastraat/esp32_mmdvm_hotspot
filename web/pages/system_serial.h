/*
 * Monitor Page
 * Real-time monitoring of MMDVM traffic and activity
 */

#ifndef WEB_SYSTEM_SERIAL_H
#define WEB_SYSTEM_SERIAL_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

String getSystemSerialPageHTML()
{
  String html;
  html.reserve(31000);
  html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Serial Monitor</title>";
  html += getSharedStyles();
  html += "<style>";
  html += ".log-container{background:var(--bg-secondary);padding:15px;border-radius:8px;margin:20px 0}";
  html += ".log{background:#000;color:#0f0;padding:15px;font-family:'Courier New',monospace;font-size:12px;height:500px;overflow-y:auto;border-radius:5px;border:1px solid #333}";
  html += ".log-controls{margin-bottom:15px;display:flex;gap:10px}";
  html += ".log-controls button{padding:8px 16px;background:var(--accent-color);color:white;border:none;border-radius:4px;cursor:pointer}";
  html += ".log-controls button:hover{opacity:0.8}";
  html += ".status-bar{padding:8px;background:var(--bg-tertiary);border-radius:4px;margin-bottom:10px;font-size:13px}";
  html += "</style>";
  html += "<script>";
  html += "let autoScroll=true;";
  html += "let updateInterval;";
  html += "function updateLogs(){";
  html += "  fetch('/api/logs').then(r=>r.json()).then(logs=>{";
  html += "    const logDiv=document.getElementById('log-output');";
  html += "    const atBottom=logDiv.scrollHeight-logDiv.clientHeight<=logDiv.scrollTop+1;";
  html += "    logDiv.innerHTML=logs.join('<br>');";
  html += "    if(autoScroll||atBottom){logDiv.scrollTop=logDiv.scrollHeight;}";
  html += "    document.getElementById('log-count').textContent='Messages: '+logs.length;";
  html += "  }).catch(e=>console.error('Failed to fetch logs:',e));";
  html += "}";
  html += "function toggleAutoScroll(){";
  html += "  autoScroll=!autoScroll;";
  html += "  document.getElementById('auto-scroll-btn').textContent=autoScroll?'Auto-Scroll: ON':'Auto-Scroll: OFF';";
  html += "}";
  html += "function copyLogs(){";
  html += "  var text=document.getElementById('log-output').innerText;";
  html += "  var ta=document.createElement('textarea');";
  html += "  ta.value=text;";
  html += "  ta.style.position='fixed';";
  html += "  ta.style.opacity='0';";
  html += "  document.body.appendChild(ta);";
  html += "  ta.select();";
  html += "  document.execCommand('copy');";
  html += "  document.body.removeChild(ta);";
  html += "  var btn=document.getElementById('copy-btn');";
  html += "  btn.textContent='Copied!';";
  html += "  setTimeout(function(){btn.textContent='Copy Logs';},2000);";
  html += "}";
  // Modal helpers
  html += "window.showModal=function(contentFn){var overlay=document.createElement('div');overlay.className='modal-overlay';var box=document.createElement('div');box.className='modal-box';contentFn(box,function(){document.body.removeChild(overlay);});overlay.appendChild(box);overlay.addEventListener('click',function(e){if(e.target===overlay)document.body.removeChild(overlay);});document.body.appendChild(overlay);return overlay;};";
  html += "window.showConfirm=function(msg,onYes){showModal(function(box,close){box.innerHTML='<h4>'+msg+'</h4>';var btns=document.createElement('div');btns.className='modal-buttons';var yes=document.createElement('button');yes.textContent='Yes';yes.className='btn btn-success';yes.onclick=function(){close();onYes();};var no=document.createElement('button');no.textContent='Cancel';no.className='btn btn-danger';no.onclick=close;btns.appendChild(yes);btns.appendChild(no);box.appendChild(btns);});};";
  html += "function clearLogs(){";
  html += "  showConfirm('Clear log buffer?',function(){";
  html += "    fetch('/api/logs/clear', {method: 'POST'}).then(()=>updateLogs());";
  html += "  });";
  html += "}";
  html += "function toggleUpdates(){";
  html += "  if(updateInterval){";
  html += "    clearInterval(updateInterval);";
  html += "    updateInterval=null;";
  html += "    document.getElementById('toggle-btn').textContent='Start Updates';";
  html += "  }else{";
  html += "    updateInterval=setInterval(updateLogs,1000);";
  html += "    document.getElementById('toggle-btn').textContent='Stop Updates';";
  html += "    updateLogs();";
  html += "  }";
  html += "}";
  html += "window.onload=function(){";
  html += "  updateLogs();";
  html += "  updateInterval=setInterval(updateLogs,1000);";
  html += "};";
  html += "</script></head>";
  html += "<body>";
  html += getNavigation("system-serialmonitor");
  html += "<div class='container'>";
  html += "<h1>Serial Monitor</h1>";
  html += "<p>Live serial output from the MMDVM modem for diagnostics and debugging.</p>";
  html += "<div class='log-container'>";
  html += "<div class='status-bar'>";
  html += "<span id='log-count'>Messages: 0</span> | ";
  html += "<span>Updates every 1 second</span>";
  html += "</div>";
  html += "<div class='log-controls'>";
  html += "<button id='toggle-btn' onclick='toggleUpdates()'>Stop Updates</button>";
  html += "<button id='auto-scroll-btn' onclick='toggleAutoScroll()'>Auto-Scroll: ON</button>";
  html += "<button onclick='updateLogs()'>Refresh Now</button>";
  html += "<button id='copy-btn' onclick='copyLogs()'>Copy Logs</button>";
  html += "<button onclick='clearLogs()' style='background:#dc3545'>Clear Buffer</button>";
  html += "</div>";
  html += "<div id='log-output' class='log'>Loading...</div>";
  html += "</div>";
  html += "</div>";
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SYSTEM_SERIAL_H
