/*
 * DAPNET Service Page
 * Configure DAPNET network client and view received paging messages.
 * DAPNET is an incoming paging service — it connects to the DAPNET network
 * and queues received calls for POCSAG transmission via the modem.
 */

#ifndef WEB_SERVICE_DAPNET_H
#define WEB_SERVICE_DAPNET_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

extern bool modePocsagEnabled;
extern bool dapnetEnabled;
extern bool pocsagServerEspNow;
extern String dapnetServer;
extern uint16_t dapnetPort;
extern String dapnetNodeCs;
extern String dapnetAuthKey;
extern String userCallsign;

String getServiceDapnetPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>DAPNET Configuration</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("service-dapnet");
  html += "<div class='container'>";
  html += "<h1>DAPNET Configuration</h1>";
  html += "<p>DAPNET is an incoming paging network — messages received from DAPNET are queued for POCSAG transmission via the modem.</p>";
  html += "<div class='admin-grid'>";

  // Card 1: DAPNET Network Settings
  {
    String nodeCs = dapnetNodeCs.length() > 0 ? dapnetNodeCs : userCallsign;
    const char* dapnetServers[] = {
      "dapnet.afu.rwth-aachen.de",
      "137.226.79.100",
      "db0dbn.ig-funk-siebengebirge.de",
      "dapnet.db0sda.ampr.org",
      "node1.dapnet-italia.it"
    };
    // Ensure a valid server is selected; fall back to first if unknown
    bool isKnownServer = false;
    for (int i = 0; i < 5; i++) {
      if (dapnetServer == String(dapnetServers[i])) { isKnownServer = true; break; }
    }
    String activeServer = isKnownServer ? dapnetServer : String(dapnetServers[0]);

    html += "<div class='card'>";
    html += "<h3>DAPNET Network</h3>";

    // Notice banner — hidden when POCSAG is enabled
    html += "<div id='dapnet-pocsag-notice' style='background:#fff3cd;border:1px solid #ffc107;border-radius:6px;padding:8px 12px;margin-bottom:10px;font-size:0.88em;color:#856404;" + String(modePocsagEnabled ? "display:none;" : "") + "'>";
    html += "&#9888; POCSAG mode must be enabled before activating DAPNET.";
    html += "</div>";

    // Enable toggle
    html += "<div class='metric'>";
    html += "<span class='metric-label'>Enable DAPNET Client:</span>";
    html += "<label class='switch'>";
    html += "<input type='checkbox' id='dapnet-enable'" + String(dapnetEnabled ? " checked" : "") + String(!modePocsagEnabled ? " disabled" : "") + ">";
    html += "<span class='slider'></span>";
    html += "</label>";
    html += "</div>";

    // Source: radio buttons (mirrors DMR source selector)
    html += "<div class='metric' style='margin-bottom:12px;'>";
    html += "<span class='metric-label'>Source:</span>";
    html += "<label style='margin-right:16px;cursor:pointer;'>";
    html += "<input type='radio' name='dapnet_source' value='server'" + String(!pocsagServerEspNow ? " checked" : "") + " onchange='updateDapnetSource()'> DAPNET Server";
    html += "</label>";
    html += "<label style='cursor:pointer;'>";
    html += "<input type='radio' name='dapnet_source' value='espnow'" + String(pocsagServerEspNow ? " checked" : "") + " onchange='updateDapnetSource()'> ESP-NOW Relay";
    html += "</label>";
    html += "</div>";

    // DAPNET-server-specific fields (hidden in ESP-NOW mode)
    html += "<div id='dapnet-server-fields'" + String(pocsagServerEspNow ? " style='display:none;'" : "") + ">";

    // Server dropdown
    html += "<div class='metric'><span class='metric-label'>Server:</span>";
    html += "<select id='dapnet_server' style='width:220px;'>";
    for (int i = 0; i < 5; i++) {
      bool sel = (activeServer == String(dapnetServers[i]));
      html += "<option value='" + String(dapnetServers[i]) + "'" + (sel ? " selected" : "") + ">" + String(dapnetServers[i]) + "</option>";
    }
    html += "</select>";
    html += "</div>";

    // Port
    html += "<div class='metric'>";
    html += "<span class='metric-label'>Port:</span>";
    html += "<input type='text' id='dapnet_port' value='" + String(dapnetPort) + "' maxlength='5' style='width:120px;' oninput=\"this.value=this.value.replace(/[^0-9]/g,'');\" onblur=\"if(!this.value||parseInt(this.value)<1)this.value='43434';\">";
    html += "</div>";

    // Callsign
    html += "<div class='metric'>";
    html += "<span class='metric-label'>Callsign:</span>";
    html += "<input type='text' id='dapnet_cs' value='" + nodeCs + "' maxlength='16' style='width:120px;'>";
    html += "</div>";

    // AuthKey (text so the saved value is always visible)
    html += "<div class='metric'>";
    html += "<span class='metric-label'>AuthKey:</span>";
    html += "<input type='password' id='dapnet_key' value='" + dapnetAuthKey + "' maxlength='64' style='width:180px;'>";
    html += "</div>";

    html += "</div>"; // end dapnet-server-fields

    html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
    html += "<button class='btn btn-success' onclick='saveDapnetSettings()'>Save &amp; Reboot</button>";
    html += "<button class='btn btn-danger' onclick='resetDapnetSettings()'>Reset to Defaults</button>";
    html += "</div>";
    html += "<div id='dapnetResult' style='margin-top:8px;font-size:0.9em;'></div>";
    html += "</div>";
  }

  // Card 2: Received Message History
  html += "<div class='card'>";
  html += "<h3>Received Messages</h3>";
  html += "<div id='historyTable' style='margin-top:6px;'><p style='color:#888;font-size:0.9em;'>Loading...</p></div>";
  html += "<div class='action-buttons-vertical' style='margin-top:10px;'>";
  html += "<button class='btn btn-primary' onclick='refreshHistory()'>Refresh</button>";
  html += "</div>";
  html += "</div>";

  html += "</div>"; // close admin-grid
  html += "<script>\n";

  // Modal helpers
  html += "function showModal(contentFn){var o=document.createElement('div');o.className='modal-overlay';var b=document.createElement('div');b.className='modal-box';contentFn(b,function(){document.body.removeChild(o);});o.appendChild(b);o.addEventListener('click',function(e){if(e.target===o)document.body.removeChild(o);});document.body.appendChild(o);}\n";
  html += "function showAlert(msg,onOk){showModal(function(b,close){b.innerHTML='<h4>'+msg+'</h4>';var btn=document.createElement('button');btn.textContent='OK';btn.className='btn btn-primary';btn.onclick=function(){close();if(onOk)onOk();};b.appendChild(btn);});}\n";
  html += "function showConfirm(msg,onYes){showModal(function(b,close){b.innerHTML='<h4>'+msg+'</h4>';var btns=document.createElement('div');btns.className='modal-buttons';var y=document.createElement('button');y.textContent='Yes';y.className='btn btn-success';y.onclick=function(){close();onYes();};var n=document.createElement('button');n.textContent='Cancel';n.className='btn btn-danger';n.onclick=close;btns.appendChild(y);btns.appendChild(n);b.appendChild(btns);});}\n";
  html += "function saveAndReboot(msg){showAlert(msg+'<br><br>The device will now reboot.',function(){fetch('/api/reboot',{method:'POST'});document.body.innerHTML='<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';setTimeout(function(){location.reload();},10000);});}\n";

  // Show/hide DAPNET server fields based on source radio selection
  html += "function updateDapnetSource(){\n";
  html += "  var espnow=document.querySelector('input[name=\"dapnet_source\"]:checked').value==='espnow';\n";
  html += "  document.getElementById('dapnet-server-fields').style.display=espnow?'none':'';\n";
  html += "}\n";

  // Save DAPNET settings
  html += "function saveDapnetSettings(){\n";
  html += "  var enable=document.getElementById('dapnet-enable').checked;\n";
  html += "  var espnow=document.querySelector('input[name=\"dapnet_source\"]:checked').value==='espnow';\n";
  html += "  var srv=espnow?'dapnet.afu.rwth-aachen.de':document.getElementById('dapnet_server').value;\n";
  html += "  var body='pocsag_espnow='+(espnow?'1':'0')\n";
  html += "    +'&dapnet_server='+encodeURIComponent(srv)\n";
  html += "    +'&dapnet_port='+encodeURIComponent(document.getElementById('dapnet_port').value)\n";
  html += "    +'&dapnet_cs='+encodeURIComponent(document.getElementById('dapnet_cs').value)\n";
  html += "    +'&dapnet_key='+encodeURIComponent(document.getElementById('dapnet_key').value);\n";
  html += "  showConfirm('Save DAPNET settings and reboot?',function(){\n";
  html += "    fetch('/api/mode-toggle',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'mode=dapnet&enable='+enable})\n";
  html += "    .then(function(){return fetch('/api/save-pocsag-settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body});})\n";
  html += "    .then(r=>r.text()).then(msg=>{saveAndReboot(msg);})\n";
  html += "    .catch(function(){showAlert('Failed to save settings.');});\n";
  html += "  });\n";
  html += "}\n";

  html += "function resetDapnetSettings(){\n";
  html += "  showConfirm('Reset DAPNET settings to defaults?',function(){\n";
  html += "    fetch('/api/reset-pocsag-settings',{method:'POST'})\n";
  html += "    .then(r=>r.text()).then(msg=>{showAlert(msg+'<br>Reload to see defaults.');}).catch(function(){showAlert('Reset failed.');});\n";
  html += "  });\n";
  html += "}\n";

  // Message history
  html += "function refreshHistory(){\n";
  html += "  fetch('/api/dapnet-history').then(r=>r.json()).then(function(d){\n";
  html += "    var tbl=document.getElementById('historyTable');\n";
  html += "    if(d.count===0){tbl.innerHTML='<p style=\\'color:#888;font-size:0.9em;\\'>No messages received yet</p>';return;}\n";
  html += "    var types=['Numeric','Alert1','Alert2','Alpha'];\n";
  html += "    var h='<table style=\\'width:100%;font-size:0.85em;border-collapse:collapse;\\'>';\n";
  html += "    h+='<tr><th style=\\'text-align:left;padding:3px;border-bottom:1px solid #ccc;\\'>Time</th>';\n";
  html += "    h+='<th style=\\'text-align:left;padding:3px;border-bottom:1px solid #ccc;\\'>RIC</th>';\n";
  html += "    h+='<th style=\\'text-align:left;padding:3px;border-bottom:1px solid #ccc;\\'>Type</th>';\n";
  html += "    h+='<th style=\\'text-align:left;padding:3px;border-bottom:1px solid #ccc;\\'>Message</th></tr>';\n";
  html += "    for(var i=d.items.length-1;i>=0;i--){\n";
  html += "      var item=d.items[i];\n";
  html += "      h+='<tr><td style=\\'padding:3px;white-space:nowrap;\\'>'+item.time+'</td>';\n";
  html += "      h+='<td style=\\'padding:3px;\\'>'+item.ric+'</td>';\n";
  html += "      h+='<td style=\\'padding:3px;\\'>'+(types[item.func]||'f'+item.func)+'</td>';\n";
  html += "      h+='<td style=\\'padding:3px;word-break:break-all;\\'>'+item.msg+'</td></tr>';\n";
  html += "    }\n";
  html += "    h+='</table>'; tbl.innerHTML=h;\n";
  html += "  }).catch(function(){document.getElementById('historyTable').innerHTML='<p style=\\'color:red;font-size:0.9em;\\'>Error loading history</p>';});\n";
  html += "}\n";
  html += "refreshHistory();\n";
  html += "setInterval(refreshHistory,10000);\n";

  html += "</script>\n";
  html += "</div>";
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SERVICE_DAPNET_H
