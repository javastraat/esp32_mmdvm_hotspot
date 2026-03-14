/*
 * POCSAG Mode Configuration Page
 * Configures the POCSAG radio modem mode (frequency, RIC, whitelist/blacklist).
 * DAPNET network settings → /service-dapnet
 * HamPager outgoing service → /service-hampager
 */

#ifndef WEB_MODE_POCSAG_H
#define WEB_MODE_POCSAG_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

extern bool modePocsagEnabled;
extern uint32_t pocsagFrequency;
extern uint32_t userDmrId;
extern uint32_t dapnetRic;
extern String pocsagWhitelist;
extern String pocsagBlacklist;
extern String userCallsign;

String getModePocsagPageHTML()
{
  String html;
  html.reserve(41000);
  html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>POCSAG Configuration</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("mode-pocsag");
  html += "<div class='container'>";
  html += "<h1>POCSAG Configuration</h1>";
  html += "<p>Configure POCSAG paging mode settings. For DAPNET network settings see <a href='/service-dapnet' style='color:var(--link-color);'>DAPNET</a>, for outgoing paging see <a href='/service-hampager' style='color:var(--link-color);'>HamPager</a>.</p>";
  html += "<div class='admin-grid'>";

  // Resolve RIC once — used by modem config and test send
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
  html += "<button class='btn btn-success' onclick='savePocsagSettings()'>Save &amp; Reboot</button>";
  html += "<button class='btn btn-danger' onclick='resetPocsagDefaults()'>Reset to Defaults</button>";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>Enable to transmit POCSAG messages and to use DAPNET/HamPager.</p>";
  html += "</div>";

  // Card 2: POCSAG Modem Configuration
  html += "<div class='card'>";
  html += "<h3>POCSAG Configuration</h3>";

  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Frequency (MHz):</span>";
  html += "<input type='text' id='pocsag_freq_mhz' value='" + String(pocsagFrequency / 1000000.0, 6) + "' style='width:120px;padding-right:8px;' oninput=\"this.value=this.value.replace(/[^0-9.]/g,'');\" placeholder='439.987500'>";
  html += "</div>";

  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>RIC:</span>";
  html += "<input type='text' id='dapnet_ric' value='" + String(ricVal) + "' maxlength='7' style='width:120px;padding-right:8px;' oninput=\"this.value=this.value.replace(/[^0-9]/g,'');if(this.value.length>7)this.value=this.value.slice(0,7);\" onblur=\"if(parseInt(this.value)<1||this.value==='')this.value='1';if(parseInt(this.value)>9999999)this.value='9999999';\">";
  html += "</div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:2px;margin-bottom:6px;'>7-digit Receiver Identity Code</p>";

  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Whitelist:</span>";
  html += "<input type='text' id='pocsag_wlist' value='" + pocsagWhitelist + "' placeholder='RIC1,RIC2,...' style='width:120px;padding-right:8px;'>";
  html += "</div>";

  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Blacklist:</span>";
  html += "<input type='text' id='pocsag_blist' value='" + pocsagBlacklist + "' placeholder='RIC1,RIC2,...' style='width:120px;padding-right:8px;'>";
  html += "</div>";

  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='savePocsagModemSettings()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetPocsagNetSettings()'>Reset to Defaults</button>";
  html += "</div>";
  html += "<div id='pocsagModemResult' style='margin-top:8px;font-size:0.9em;'></div>";
  html += "</div>";

  // Card 3: Send Test POCSAG Message
  html += "<div class='card'>";
  html += "<h3>Send Test POCSAG Message</h3>";
  html += "<form id='pocsagTestForm' onsubmit='return sendPocsagTest(event)'>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>RIC:</span>";
  html += "<input type='text' id='pocsagRic' name='ric' placeholder='" + String(ricVal) + "' value='" + String(ricVal) + "' maxlength='7' required style='width:120px;padding-right:8px;' oninput=\"this.value=this.value.replace(/[^0-9]/g,'');if(this.value.length>7)this.value=this.value.slice(0,7);\" onblur=\"if(parseInt(this.value)<1||this.value==='')this.value='1';if(parseInt(this.value)>9999999)this.value='9999999';\">";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Encoding:</span>";
  html += "<select id='pocsagEnc' name='encoding' style='width:120px;'>";
  html += "<option value='alpha' selected>Alphanumeric</option>";
  html += "<option value='numeric'>Numeric</option>";
  html += "</select>";
  html += "</div>";
  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Message:</span>";
  html += "<input type='text' id='pocsagMsg' name='message' placeholder='Test message' value='Test message' maxlength='80' required style='width:200px;padding-right:8px;'>";
  html += "</div>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button type='submit' class='btn btn-success'>Send Message</button>";
  html += "</div>";
  html += "<div id='pocsagTestResult' style='margin-top:10px;font-size:0.95em;'></div>";
  html += "</form>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>Frequency: <b>" + String(pocsagFrequency / 1000000.0, 6) + " MHz</b></p>";
  html += "</div>";

  // Card 4: POCSAG Queue Monitor
  html += "<div class='card'>";
  html += "<h3>POCSAG Queue</h3>";
  html += "<div class='metric'><span class='metric-label'>Pending:</span><span class='metric-value' id='queueCount'>-</span></div>";
  html += "<div id='queueTable' style='margin-top:10px;'></div>";
  html += "<div class='action-buttons-vertical' style='margin-top:10px;'>";
  html += "<button class='btn btn-primary' onclick='refreshQueue()'>Refresh</button>";
  html += "</div>";
  html += "</div>";

  // Card 5: Last 15 Transmitted POCSAG
  html += "<div class='card'>";
  html += "<div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;'>";
  html += "<h3 style='margin:0;'>Last 15 Transmitted</h3>";
  html += "<span id='pocsag-tx-count' style='font-size:0.8em;color:#888;'></span>";
  html += "</div>";
  html += "<div style='overflow-x:auto;'>";
  html += "<table style='width:100%;border-collapse:collapse;font-size:0.85em;'>";
  html += "<thead><tr style='border-bottom:1px solid #ccc;'>";
  html += "<th style='text-align:left;padding:4px 6px;white-space:nowrap;'>Time</th>";
  html += "<th style='text-align:left;padding:4px 6px;'>RIC</th>";
  html += "<th style='text-align:left;padding:4px 6px;'>Message</th>";
  html += "</tr></thead>";
  html += "<tbody id='pocsag-tx-body'><tr><td colspan='3' style='padding:8px 6px;color:#888;'>No messages yet</td></tr></tbody>";
  html += "</table></div>";
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
  html += "function showModal(contentFn){\n";
  html += "  var overlay=document.createElement('div');\n";
  html += "  overlay.className='modal-overlay';\n";
  html += "  var box=document.createElement('div');\n";
  html += "  box.className='modal-box';\n";
  html += "  contentFn(box,function(){document.body.removeChild(overlay);});\n";
  html += "  overlay.appendChild(box);\n";
  html += "  overlay.addEventListener('click',function(e){if(e.target===overlay)document.body.removeChild(overlay);});\n";
  html += "  document.body.appendChild(overlay);\n";
  html += "}\n";
  html += "function showAlert(msg,onOk){\n";
  html += "  showModal(function(box,close){\n";
  html += "    box.innerHTML='<h4>'+msg+'</h4>';\n";
  html += "    var btns=document.createElement('div');btns.className='modal-buttons';\n";
  html += "    var ok=document.createElement('button');ok.textContent='OK';ok.className='btn btn-primary';\n";
  html += "    ok.onclick=function(){close();if(onOk)onOk();};\n";
  html += "    btns.appendChild(ok);box.appendChild(btns);\n";
  html += "  });\n";
  html += "}\n";
  html += "function showConfirm(msg,onYes){\n";
  html += "  showModal(function(box,close){\n";
  html += "    box.innerHTML='<h4>'+msg+'</h4>';\n";
  html += "    var btns=document.createElement('div');btns.className='modal-buttons';\n";
  html += "    var yes=document.createElement('button');yes.textContent='Yes';yes.className='btn btn-success';\n";
  html += "    yes.onclick=function(){close();onYes();};\n";
  html += "    var no=document.createElement('button');no.textContent='Cancel';no.className='btn btn-danger';\n";
  html += "    no.onclick=close;\n";
  html += "    btns.appendChild(yes);btns.appendChild(no);box.appendChild(btns);\n";
  html += "  });\n";
  html += "}\n";
  html += "function saveAndReboot(msg){\n";
  html += "  showAlert(msg+'<br><br>The device will now reboot.',function(){\n";
  html += "    fetch('/api/reboot',{method:'POST'});\n";
  html += "    document.body.innerHTML='<div style=\"display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:24px;\">Rebooting... Page will reload in 10 seconds.</div>';\n";
  html += "    setTimeout(function(){location.reload();},10000);\n";
  html += "  });\n";
  html += "}\n";

  // Card 1: Enable toggle
  html += "function savePocsagSettings(){\n";
  html += "  var enable=document.getElementById('mode-pocsag').checked;\n";
  html += "  showConfirm('Save POCSAG settings and reboot?',function(){\n";
  html += "    fetch('/api/mode-toggle',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'mode=pocsag&enable='+enable})\n";
  html += "    .then(r=>r.text()).then(msg=>{saveAndReboot(msg);})\n";
  html += "    .catch(function(){showAlert('Failed to save settings.');});\n";
  html += "  });\n";
  html += "}\n";
  html += "function resetPocsagDefaults(){\n";
  html += "  showConfirm('Reset POCSAG settings to defaults and reboot?',function(){\n";
  html += "    fetch('/api/mode-toggle',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'mode=pocsag&enable=false'})\n";
  html += "    .then(r=>r.text()).then(msg=>{saveAndReboot(msg);})\n";
  html += "    .catch(function(){showAlert('Failed to reset settings.');});\n";
  html += "  });\n";
  html += "}\n";

  // Card 2: Modem settings
  html += "function savePocsagModemSettings(){\n";
  html += "  var freqHz=Math.round(parseFloat(document.getElementById('pocsag_freq_mhz').value)*1000000);\n";
  html += "  var body='dapnet_ric='+encodeURIComponent(document.getElementById('dapnet_ric').value)\n";
  html += "    +'&pocsag_freq='+encodeURIComponent(freqHz)\n";
  html += "    +'&pocsag_wlist='+encodeURIComponent(document.getElementById('pocsag_wlist').value)\n";
  html += "    +'&pocsag_blist='+encodeURIComponent(document.getElementById('pocsag_blist').value);\n";
  html += "  fetch('/api/save-pocsag-settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})\n";
  html += "    .then(r=>r.text()).then(msg=>{document.getElementById('pocsagModemResult').innerHTML='<span style=\\'color:green\\'>'+msg+'</span>';})\n";
  html += "    .catch(function(){document.getElementById('pocsagModemResult').innerHTML='<span style=\\'color:red\\'>Save failed</span>';});\n";
  html += "}\n";
  html += "function resetPocsagNetSettings(){\n";
  html += "  showConfirm('Reset POCSAG settings to defaults?',function(){\n";
  html += "    fetch('/api/reset-pocsag-settings',{method:'POST'})\n";
  html += "    .then(r=>r.text()).then(msg=>{showAlert(msg+'<br>Reload to see defaults.');})\n";
  html += "    .catch(function(){showAlert('Reset failed.');});\n";
  html += "  });\n";
  html += "}\n";

  // Card 3: Send test POCSAG message
  html += "function sendPocsagTest(e){\n";
  html += "  e.preventDefault();\n";
  html += "  var ric=document.getElementById('pocsagRic').value;\n";
  html += "  var msg=document.getElementById('pocsagMsg').value;\n";
  html += "  var enc=document.getElementById('pocsagEnc').value;\n";
  html += "  var result=document.getElementById('pocsagTestResult');\n";
  html += "  result.innerHTML='Sending...';\n";
  html += "  fetch('/api/send-pocsag',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ric='+encodeURIComponent(ric)+'&message='+encodeURIComponent(msg)+'&encoding='+enc})\n";
  html += "  .then(r=>r.json())\n";
  html += "  .then(j=>{\n";
  html += "    if(j.success)result.innerHTML='<span style=\\'color:green\\'>Message sent!</span>';\n";
  html += "    else result.innerHTML='<span style=\\'color:red\\'>Error: '+(j.error||'Unknown error')+'</span>';\n";
  html += "  })\n";
  html += "  .catch(function(){result.innerHTML='<span style=\\'color:red\\'>Request failed</span>';});\n";
  html += "  return false;\n";
  html += "}\n";

  // Card 5: TX history
  html += "function pollPocsagTx(){\n";
  html += "  fetch('/api/pocsag-tx-history').then(function(r){return r.json();}).then(function(d){\n";
  html += "    var cnt=document.getElementById('pocsag-tx-count');if(cnt)cnt.textContent=d.count+' / 15';\n";
  html += "    var tbody=document.getElementById('pocsag-tx-body');if(!tbody)return;\n";
  html += "    if(!d.items||d.items.length===0){tbody.innerHTML='<tr><td colspan=\"3\" style=\"padding:8px 6px;color:#888;\">No messages yet</td></tr>';return;}\n";
  html += "    var rows='';\n";
  html += "    for(var i=0;i<d.items.length;i++){\n";
  html += "      var p=d.items[i];var bg=(i%2===0)?'':'background:rgba(128,128,128,0.05);';\n";
  html += "      rows+='<tr style=\"'+bg+'\"><td style=\"padding:4px 6px;color:#888;white-space:nowrap;\">'+(p.time||'')+'</td>';\n";
  html += "      rows+='<td style=\"padding:4px 6px;font-weight:bold;white-space:nowrap;\">'+(p.ric||'')+'</td>';\n";
  html += "      rows+='<td style=\"padding:4px 6px;word-break:break-word;\">'+(p.msg||'')+'</td></tr>';\n";
  html += "    }\n";
  html += "    tbody.innerHTML=rows;\n";
  html += "  }).catch(function(){});\n";
  html += "}\n";
  html += "setInterval(pollPocsagTx,5000);\n";
  html += "pollPocsagTx();\n";

  // Card 4: Queue monitor
  html += "function refreshQueue(){\n";
  html += "  fetch('/api/pocsag-queue').then(r=>r.json()).then(function(d){\n";
  html += "    document.getElementById('queueCount').textContent=d.count+' / '+d.capacity;\n";
  html += "    var tbl=document.getElementById('queueTable');\n";
  html += "    if(d.count===0){tbl.innerHTML='<p style=\\'color:#888;font-size:0.9em;\\'>Queue is empty</p>';return;}\n";
  html += "    var html='<table style=\\'width:100%;font-size:0.85em;border-collapse:collapse;\\'>';\n";
  html += "    html+='<tr><th style=\\'text-align:left;padding:3px;border-bottom:1px solid #ccc;\\'>RIC</th>';\n";
  html += "    html+='<th style=\\'text-align:left;padding:3px;border-bottom:1px solid #ccc;\\'>Type</th>';\n";
  html += "    html+='<th style=\\'text-align:left;padding:3px;border-bottom:1px solid #ccc;\\'>Message</th></tr>';\n";
  html += "    var types=['Numeric','Alert1','Alert2','Alpha'];\n";
  html += "    for(var i=0;i<d.items.length;i++){\n";
  html += "      var item=d.items[i];\n";
  html += "      html+='<tr><td style=\\'padding:3px;\\'>'+item.ric+'</td>';\n";
  html += "      html+='<td style=\\'padding:3px;\\'>'+(types[item.func]||item.func)+'</td>';\n";
  html += "      html+='<td style=\\'padding:3px;word-break:break-all;\\'>'+item.msg+'</td></tr>';\n";
  html += "    }\n";
  html += "    html+='</table>'; tbl.innerHTML=html;\n";
  html += "  }).catch(function(){document.getElementById('queueCount').textContent='error';});\n";
  html += "}\n";
  html += "refreshQueue();\n";
  html += "setInterval(refreshQueue,2000);\n";

  // Save All & Reboot
  html += "function saveAllPocsagSettings(){\n";
  html += "  showConfirm('Save all POCSAG settings and reboot?',function(){\n";
  html += "    var post=function(url,body){return fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body});};\n";
  html += "    var pocsagOn=document.getElementById('mode-pocsag').checked;\n";
  html += "    var freqHz=Math.round(parseFloat(document.getElementById('pocsag_freq_mhz').value)*1000000);\n";
  html += "    var body='pocsag_freq='+encodeURIComponent(freqHz)\n";
  html += "      +'&dapnet_ric='+encodeURIComponent(document.getElementById('dapnet_ric').value)\n";
  html += "      +'&pocsag_wlist='+encodeURIComponent(document.getElementById('pocsag_wlist').value)\n";
  html += "      +'&pocsag_blist='+encodeURIComponent(document.getElementById('pocsag_blist').value);\n";
  html += "    post('/api/mode-toggle','mode=pocsag&enable='+pocsagOn)\n";
  html += "      .then(function(){return post('/api/save-pocsag-settings',body);})\n";
  html += "      .then(function(){saveAndReboot('All POCSAG settings saved.');})\n";
  html += "      .catch(function(){showAlert('Failed to save settings.');});\n";
  html += "  });\n";
  html += "}\n";

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
