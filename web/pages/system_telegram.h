/*
 * System Telegram Page
 * Telegram Bot integration configuration
 */

#ifndef WEB_SYSTEM_TELEGRAM_H
#define WEB_SYSTEM_TELEGRAM_H

#include <Arduino.h>
#include "web/include/styles.h"
#include "web/include/navigation.h"
#include "web/include/utils.h"

extern bool telegramEnabled;
extern String telegramBotToken;
extern String telegramChatId;
extern bool telegramRicForwardEnabled;
extern String telegramRicList;
extern uint16_t telegramRicCooldown;

String getSystemTelegramPageHTML()
{
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Telegram Configuration</title>";
  html += getSharedStyles();
  html += "</head><body>";
  html += getNavigation("system-telegram");

  html += "<div class='container'>";
  html += "<h1>Telegram Configuration</h1>";
  html += "<p>Configure Telegram Bot integration for alerts and POCSAG message forwarding</p>";
  html += "<div class='admin-grid'>";

  // ── Card 1: Enable / Disable ─────────────────────────────────────────────
  html += "<div class='card'>";
  html += "<h3>Telegram Integration</h3>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enable Telegram:</span>";
  html += "<label class='switch'><input type='checkbox' id='tg-enabled'" + String(telegramEnabled ? " checked" : "") + "><span class='slider'></span></label>";
  html += "</div>";
  html += "<div class='metric'><span class='metric-label'>Current Status:</span><span class='metric-value'>" + String(telegramEnabled ? "Enabled" : "Disabled") + "</span></div>";
  html += "<p style='font-size:0.85em;color:#666;margin-top:10px;'>Enable to allow sending messages via Telegram Bot API. Bot credentials must be configured in the next card.</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveTgService()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetTgService()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  // ── Card 2: Bot Credentials ───────────────────────────────────────────────
  html += "<div class='card'>";
  html += "<h3>Bot Settings</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Get your bot token from @BotFather on Telegram. Find your chat ID by messaging the bot and visiting the getUpdates API endpoint.</p>";
  html += "<form onsubmit=\"return false;\">";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Bot Token:</span>";
  html += "<input type='password' id='tg-token' value='" + telegramBotToken + "' placeholder='1234567890:AAF...' style='width:160px;padding-right:8px;font-family:monospace;' autocomplete='off'>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Chat ID:</span>";
  html += "<input type='text' id='tg-chatid' value='" + telegramChatId + "' placeholder='123456789' style='width:160px;padding-right:8px;'>";
  html += "</div>";
  html += "</form>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveTgBot()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetTgBot()'>Clear Credentials</button>";
  html += "</div>";
  html += "</div>";

  // ── Card 3: Test Send ─────────────────────────────────────────────────────
  html += "<div class='card'>";
  html += "<h3>Test Message</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>Send a test message to verify your bot token and chat ID are correct. Telegram must be enabled and credentials saved first.</p>";
  html += "<div id='tg-test-result' style='margin-bottom:10px;min-height:20px;font-size:0.9em;'></div>";
  html += "<div class='action-buttons-vertical'>";
  html += "<button class='btn btn-primary' onclick='sendTgTest()'>Send Test Message</button>";
  html += "</div>";
  html += "</div>";

  // ── Card 4: RIC Forward Monitor ───────────────────────────────────────────
  html += "<div class='card'>";
  html += "<h3>POCSAG &rarr; Telegram Forward</h3>";
  html += "<p style='font-size:0.85em;color:#666;margin-bottom:10px;'>When enabled, incoming POCSAG messages for the listed RICs are automatically forwarded to your Telegram chat. Useful when you are not carrying your pager.</p>";
  html += "<p style='font-size:0.82em;color:#888;margin-bottom:10px;'><em>Note: This list is independent of the POCSAG whitelist/blacklist.</em></p>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Enable Forwarding:</span>";
  html += "<label class='switch'><input type='checkbox' id='tg-ric-en'" + String(telegramRicForwardEnabled ? " checked" : "") + "><span class='slider'></span></label>";
  html += "</div>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>RIC List:</span>";
  html += "<input type='text' id='tg-ric-list' value='" + telegramRicList + "' placeholder='2041152,1234567' style='width:160px;padding-right:8px;'>";
  html += "</div>";
  html += "<p style='font-size:0.82em;color:#888;margin-top:4px;margin-bottom:8px;'>Comma-separated list of RICs to monitor</p>";
  html += "<div class='metric'>";
  html += "<span class='metric-label'>Cooldown (s):</span>";
  html += "<input type='number' id='tg-cooldown' value='" + String(telegramRicCooldown) + "' min='10' max='3600' style='width:80px;padding-right:8px;'>";
  html += "</div>";
  html += "<p style='font-size:0.82em;color:#888;margin-top:4px;'>Minimum seconds between forwards per RIC (10&ndash;3600)</p>";
  html += "<div class='action-buttons-vertical' style='margin-top:15px;'>";
  html += "<button class='btn btn-success' onclick='saveTgRic()'>Save</button>";
  html += "<button class='btn btn-danger' onclick='resetTgRic()'>Reset to Default</button>";
  html += "</div>";
  html += "</div>";

  html += "</div>"; // close admin-grid

  // ── JavaScript ────────────────────────────────────────────────────────────
  html += "<script>";

  // Shared modal helpers (matching system_mqtt / system_wifi style)
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

  // Service
  html += "function saveTgService(){";
  html += "var en=document.getElementById('tg-enabled').checked?'1':'0';";
  html += "showConfirm('Save Telegram service setting?',function(){";
  html += "fetch('/api/save-telegram-service',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'enabled='+en})";
  html += ".then(r=>r.text()).then(msg=>{showAlert(msg);});});";
  html += "}";

  html += "function resetTgService(){";
  html += "showConfirm('Reset Telegram service to default?',function(){";
  html += "fetch('/api/reset-telegram-service',{method:'POST'}).then(r=>r.text()).then(msg=>{showAlert(msg);location.reload();});});";
  html += "}";

  // Bot credentials
  html += "function saveTgBot(){";
  html += "var token=document.getElementById('tg-token').value.trim();";
  html += "var chatid=document.getElementById('tg-chatid').value.trim();";
  html += "if(token.length<10){showAlert('Bot token is too short');return;}";
  html += "if(!chatid){showAlert('Chat ID cannot be empty');return;}";
  html += "showConfirm('Save bot credentials?',function(){";
  html += "fetch('/api/save-telegram-bot',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},";
  html += "body:'token='+encodeURIComponent(token)+'&chatid='+encodeURIComponent(chatid)})";
  html += ".then(r=>r.text()).then(msg=>{showAlert(msg);});});";
  html += "}";

  html += "function resetTgBot(){";
  html += "showConfirm('Clear bot credentials?',function(){";
  html += "fetch('/api/reset-telegram-bot',{method:'POST'}).then(r=>r.text()).then(msg=>{";
  html += "document.getElementById('tg-token').value='';";
  html += "document.getElementById('tg-chatid').value='';";
  html += "showAlert(msg);});});";
  html += "}";

  // Test send
  html += "function sendTgTest(){";
  html += "var el=document.getElementById('tg-test-result');";
  html += "el.style.color='#666';el.textContent='Sending...';";
  html += "fetch('/api/telegram-test',{method:'POST'}).then(r=>r.text()).then(msg=>{";
  html += "el.style.color=msg.startsWith('OK')?'#2e7d32':'#c62828';";
  html += "el.textContent=msg;});";
  html += "}";

  // RIC forward
  html += "function saveTgRic(){";
  html += "var en=document.getElementById('tg-ric-en').checked?'1':'0';";
  html += "var list=document.getElementById('tg-ric-list').value.trim();";
  html += "var cd=document.getElementById('tg-cooldown').value;";
  html += "if(cd<10||cd>3600){showAlert('Cooldown must be 10-3600 seconds');return;}";
  html += "showConfirm('Save RIC forward settings?',function(){";
  html += "fetch('/api/save-telegram-ric',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},";
  html += "body:'ric_en='+en+'&ric_list='+encodeURIComponent(list)+'&cooldown='+cd})";
  html += ".then(r=>r.text()).then(msg=>{showAlert(msg);});});";
  html += "}";

  html += "function resetTgRic(){";
  html += "showConfirm('Reset RIC forward settings to default?',function(){";
  html += "fetch('/api/reset-telegram-ric',{method:'POST'}).then(r=>r.text()).then(msg=>{showAlert(msg);location.reload();});});";
  html += "}";

  html += "</script>";

  html += "</div>"; // close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_SYSTEM_TELEGRAM_H
