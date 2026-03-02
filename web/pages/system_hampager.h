/*
 * HamPager Service Page
 * Configure HamPager credentials and send outgoing DAPNET paging messages
 * via the HamPager REST API (hampager.de).
 */

#ifndef WEB_SYSTEM_HAMPAGER_H
#define WEB_SYSTEM_HAMPAGER_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

extern String userCallsign;
extern String hampagerUser;
extern String hampagerPassword;
extern String hampagerTxGroup;

String getSystemHampagerPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>HamPager Configuration</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("system-hampager");
  html += "<div class='container'>";
  html += "<h1>HamPager</h1>";
  html += "<p>Send outgoing paging messages to any callsign via the <a href='http://hampager.de' target='_blank' style='color:var(--link-color);'>HamPager</a> REST API.</p>";
  html += "<div class='admin-grid'>";

  // Card 1: HamPager Credentials
  html += "<div class='card'>";
  html += "<h3>Credentials</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Your HamPager account credentials.</p>";

  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Username:</span>";
  html += "<input type='text' id='hp_user' value='" + hampagerUser + "' maxlength='32' autocomplete='username' style='width:130px;padding-right:8px;' placeholder='your-callsign'>";
  html += "</div>";

  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Password:</span>";
  html += "<input type='password' id='hp_pass' value='" + hampagerPassword + "' maxlength='64' autocomplete='current-password' style='width:130px;padding-right:8px;' placeholder='hampager password'>";
  html += "</div>";

  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>TX Group:</span>";
  html += "<input type='text' id='hp_txg' value='" + hampagerTxGroup + "' maxlength='32' style='width:130px;padding-right:8px;' placeholder='all'>";
  html += "</div>";
  html += "<p style='font-size:0.80em;color:#888;margin-top:2px;margin-bottom:8px;'>E.g. <code>all</code>, <code>pa-nh</code>, <code>dl-all</code></p>";

  html += "<div class='action-buttons-vertical' style='margin-top:12px;'>";
  html += "<button class='btn btn-success' onclick='saveHampagerSettings()'>Save Credentials</button>";
  html += "<button class='btn btn-danger' onclick='resetHampagerSettings()'>Reset to Defaults</button>";
  html += "</div>";
  html += "<div id='hampagerSaveResult' style='margin-top:6px;font-size:0.9em;'></div>";
  html += "</div>";

  // Card 2: Send Message
  html += "<div class='card'>";
  html += "<h3>Send Message</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Send a paging message to any callsign via HamPager.</p>";

  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>To Callsign:</span>";
  html += "<input type='text' id='hp_dest' value='" + userCallsign + "' maxlength='16' style='width:130px;padding-right:8px;' placeholder='N0CALL'>";
  html += "</div>";

  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>TX Group:</span>";
  html += "<input type='text' id='hp_send_txg' value='" + hampagerTxGroup + "' maxlength='32' style='width:130px;padding-right:8px;' placeholder='all'>";
  html += "</div>";

  html += "<div class='metric' style='position:relative;'>";
  html += "<span class='metric-label'>Message:</span>";
  html += "<input type='text' id='hp_msg' value='' maxlength='160' style='width:200px;padding-right:8px;' placeholder='Your message here'>";
  html += "</div>";

  html += "<div class='action-buttons-vertical' style='margin-top:12px;'>";
  html += "<button class='btn btn-primary' onclick='sendHampager()'>Send via HamPager</button>";
  html += "</div>";
  html += "<div id='hampagerSendResult' style='margin-top:6px;font-size:0.9em;'></div>";
  html += "</div>";

  html += "</div>"; // close admin-grid
  html += "<script>\n";

  // Modal helpers
  html += "function showModal(contentFn){var o=document.createElement('div');o.className='modal-overlay';var b=document.createElement('div');b.className='modal-box';contentFn(b,function(){document.body.removeChild(o);});o.appendChild(b);o.addEventListener('click',function(e){if(e.target===o)document.body.removeChild(o);});document.body.appendChild(o);}\n";
  html += "function showAlert(msg,onOk){showModal(function(b,close){b.innerHTML='<h4>'+msg+'</h4>';var btn=document.createElement('button');btn.textContent='OK';btn.className='btn btn-primary';btn.onclick=function(){close();if(onOk)onOk();};b.appendChild(btn);});}\n";
  html += "function showConfirm(msg,onYes){showModal(function(b,close){b.innerHTML='<h4>'+msg+'</h4>';var btns=document.createElement('div');btns.className='modal-buttons';var y=document.createElement('button');y.textContent='Yes';y.className='btn btn-success';y.onclick=function(){close();onYes();};var n=document.createElement('button');n.textContent='Cancel';n.className='btn btn-danger';n.onclick=close;btns.appendChild(y);btns.appendChild(n);b.appendChild(btns);});}\n";

  // Save credentials
  html += "function saveHampagerSettings(){\n";
  html += "  var body='hp_user='+encodeURIComponent(document.getElementById('hp_user').value)\n";
  html += "    +'&hp_pass='+encodeURIComponent(document.getElementById('hp_pass').value)\n";
  html += "    +'&hp_txg='+encodeURIComponent(document.getElementById('hp_txg').value);\n";
  html += "  fetch('/api/save-hampager-settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})\n";
  html += "    .then(r=>r.text()).then(msg=>{document.getElementById('hampagerSaveResult').innerHTML='<span style=\\'color:green\\'>'+msg+'</span>';})\n";
  html += "    .catch(function(){document.getElementById('hampagerSaveResult').innerHTML='<span style=\\'color:red\\'>Save failed</span>';});\n";
  html += "}\n";

  html += "function resetHampagerSettings(){\n";
  html += "  showConfirm('Reset HamPager settings to defaults?',function(){\n";
  html += "    fetch('/api/reset-hampager-settings',{method:'POST'})\n";
  html += "    .then(r=>r.text()).then(msg=>{document.getElementById('hampagerSaveResult').innerHTML='<span style=\\'color:green\\'>'+msg+'</span>';})\n";
  html += "    .catch(function(){document.getElementById('hampagerSaveResult').innerHTML='<span style=\\'color:red\\'>Reset failed</span>';});\n";
  html += "  });\n";
  html += "}\n";

  // Send message
  html += "function sendHampager(){\n";
  html += "  var dest=document.getElementById('hp_dest').value.trim();\n";
  html += "  var msg=document.getElementById('hp_msg').value.trim();\n";
  html += "  var txg=document.getElementById('hp_send_txg').value.trim();\n";
  html += "  var result=document.getElementById('hampagerSendResult');\n";
  html += "  if(!dest||!msg){result.innerHTML='<span style=\\'color:red\\'>Callsign and message are required</span>';return;}\n";
  html += "  result.innerHTML='Sending...';\n";
  html += "  var body='callsign='+encodeURIComponent(dest)+'&text='+encodeURIComponent(msg)+'&txg='+encodeURIComponent(txg);\n";
  html += "  fetch('/api/send-hampager',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})\n";
  html += "    .then(function(r){\n";
  html += "      return r.json().then(function(j){\n";
  html += "        if(r.ok){\n";
  html += "          result.innerHTML='<span style=\\'color:green\\'>Sent to '+dest+'!</span>';\n";
  html += "        } else {\n";
  html += "          var errMsg='Send failed';\n";
  html += "          if(j.violations&&j.violations.length>0){\n";
  html += "            var v=j.violations[0];\n";
  html += "            errMsg=(v.constraint==='ValidCallSignNames')?'Callsign <b>'+dest+'</b> is not registered in DAPNET.':(v.message||j.message||'Unknown error');\n";
  html += "          } else if(j.message){errMsg=j.message;}\n";
  html += "          result.innerHTML='<span style=\\'color:red\\'>'+errMsg+'</span>';\n";
  html += "          showAlert(errMsg);\n";
  html += "        }\n";
  html += "      });\n";
  html += "    })\n";
  html += "    .catch(function(){result.innerHTML='<span style=\\'color:red\\'>Request failed</span>';});\n";
  html += "}\n";

  html += "</script>\n";
  html += "</div>";
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SYSTEM_HAMPAGER_H
