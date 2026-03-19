#pragma once
#include "styles.h"
#include "navigation.h"

// ── ESP-NOW page (/espnow): POCSAG RIC settings ───────────────────────────
static const char PAGE_ESPNOW[] PROGMEM =
  "<!DOCTYPE html><html lang=\"en\">"
  "<head>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Ulanzi ESP-NOW</title>"
  "<style>" COMMON_CSS "</style>"
  THEME_INIT_SCRIPT
  "</head><body>"
  NAV_BAR
  NAV_LIVE_MODAL
  R"html(
<div class="container"><div class="grid">

  <!-- Card 1: POCSAG RIC Settings -->
  <div class="card">
    <h3>POCSAG RIC Settings</h3>

    <div style="margin-bottom:14px">
      <div style="font-size:.82em;color:var(--text-muted);margin-bottom:5px">
        Time beacon RIC
        <span style="font-size:.9em;color:var(--text-muted)">— carries the clock sync message</span>
      </div>
      <input type="number" id="time-ric" min="0" max="2097151" value="224"
             style="width:100%;background:var(--bg-secondary);color:var(--text-color);
                    border:1px solid var(--border-color);border-radius:4px;
                    padding:5px 8px;font-size:1em;box-sizing:border-box">
    </div>

    <div style="margin-bottom:14px">
      <div style="font-size:.82em;color:var(--text-muted);margin-bottom:5px">
        Callsign RIC
        <span style="font-size:.9em;color:var(--text-muted)">— trailing digits are stripped before display</span>
      </div>
      <input type="number" id="call-ric" min="0" max="2097151" value="8"
             style="width:100%;background:var(--bg-secondary);color:var(--text-color);
                    border:1px solid var(--border-color);border-radius:4px;
                    padding:5px 8px;font-size:1em;box-sizing:border-box">
    </div>

    <div style="margin-bottom:14px">
      <div style="font-size:.82em;color:var(--text-muted);margin-bottom:5px">
        Excluded RICs
        <span style="font-size:.9em;color:var(--text-muted)">— never shown on display, comma-separated</span>
      </div>
      <input type="text" id="excl-rics" maxlength="191"
             placeholder="e.g. 224,208,200"
             style="width:100%;background:var(--bg-secondary);color:var(--text-color);
                    border:1px solid var(--border-color);border-radius:4px;
                    padding:5px 8px;font-size:.92em;font-family:monospace;box-sizing:border-box">
      <div style="font-size:.75em;color:var(--text-muted);margin-top:4px">Max 16 RICs. Digits and commas only.</div>
    </div>

    <div style="display:flex;align-items:center;justify-content:space-between;gap:10px">
      <span id="ric-status" style="font-size:.82em;color:#4caf50;min-height:1em"></span>
      <button onclick="saveRics()" class="btn btn-info">Save</button>
    </div>
  </div>

  <!-- Card 2: Protocol Modes -->
  <div class="card">
    <h3>Protocol Modes</h3>

    <!-- POCSAG — active -->
    <div style="display:flex;align-items:center;justify-content:space-between;
                padding:8px 0;border-bottom:1px solid var(--border-color)">
      <div>
        <div style="font-weight:600;font-size:.92em">POCSAG</div>
        <div style="font-size:.78em;color:var(--text-muted)">POCSAG pages via ESP-NOW</div>
      </div>
      <label class="switch">
        <input type="checkbox" id="tog-pocsag" onchange="saveMode()">
        <span class="slider"></span>
      </label>
    </div>

    <!-- DMR — disabled (compile-time off) -->
    <div style="display:flex;align-items:center;justify-content:space-between;
                padding:8px 0;border-bottom:1px solid var(--border-color);opacity:.4">
      <div>
        <div style="font-weight:600;font-size:.92em">DMR</div>
        <div style="font-size:.78em;color:var(--text-muted)">Raw DMRD Homebrew via ESP-NOW</div>
      </div>
      <label class="switch" style="pointer-events:none">
        <input type="checkbox" disabled>
        <span class="slider"></span>
      </label>
    </div>

    <!-- ESP-NOW v2 — disabled (coming soon) -->
    <div style="display:flex;align-items:center;justify-content:space-between;
                padding:8px 0;opacity:.4">
      <div>
        <div style="font-weight:600;font-size:.92em">
          ESP-NOW v2
          <span style="font-size:.72em;background:#555;color:#ccc;
                       border-radius:3px;padding:1px 5px;margin-left:5px;
                       vertical-align:middle">soon</span>
        </div>
        <div style="font-size:.78em;color:var(--text-muted)">Extended v2 protocol — coming soon</div>
      </div>
      <label class="switch" style="pointer-events:none">
        <input type="checkbox" disabled>
        <span class="slider"></span>
      </label>
    </div>

    <div id="mode-status" style="font-size:.82em;color:#4caf50;min-height:1.2em;padding-top:6px"></div>
  </div>

  <!-- Card 3: Received Messages -->
  <div class="card">
    <h3>Received Messages</h3>
    <div class="metric" style="border-bottom:1px solid var(--border-color);margin-bottom:8px">
      <span class="metric-label">POCSAG received</span>
      <span class="metric-value" id="poc-count">-</span>
    </div>
    <div id="msg-log"><span style="color:var(--text-muted);font-size:.85em">No messages yet.</span></div>
  </div>

</div></div>
)html"
  "<script>" COMMON_JS NAV_LIVE_JS "</script>"
  R"html(
<script>
(function init(){
  pollStatus();
  setInterval(pollStatus, 3000);
  function pollStatus(){
    fetch('/api/status').then(function(r){return r.json();}).then(function(d){
      document.getElementById('h1').textContent=d.hostname;
      document.getElementById('sub').textContent=d.ip;
      document.getElementById('poc-count').textContent=d.pocsag_count||0;
      var log=d.pocsag_log||[];
      var el=document.getElementById('msg-log');
      if(!log.length){
        el.innerHTML='<span style="color:var(--text-muted);font-size:.85em">No messages yet.</span>';
      } else {
        var html='<table style="width:100%;border-collapse:collapse;font-size:.85em">';
        for(var i=0;i<log.length;i++){
          html+='<tr style="border-bottom:1px solid var(--border-color)">'
            +'<td style="color:var(--text-muted);white-space:nowrap;padding:4px 10px 4px 0;vertical-align:top">RIC '+log[i].ric+'</td>'
            +'<td style="padding:4px 0;word-break:break-all;font-family:monospace">'+escHtml(log[i].msg)+'</td>'
            +'</tr>';
        }
        html+='</table>';
        el.innerHTML=html;
      }
    }).catch(function(){});
  }
  function escHtml(s){
    return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
  }
  fetch('/api/espnow').then(function(r){return r.json();}).then(function(d){
    document.getElementById('time-ric').value=d.time_ric||224;
    document.getElementById('call-ric').value=d.call_ric||8;
    document.getElementById('excl-rics').value=(d.excl_rics||[]).join(',');
  }).catch(function(){});
  fetch('/api/espnow/modes').then(function(r){return r.json();}).then(function(d){
    document.getElementById('tog-pocsag').checked=d.pocsag;
  }).catch(function(){});
})();
function saveMode(){
  var body='pocsag='+(document.getElementById('tog-pocsag').checked?1:0);
  fetch('/api/espnow/modes',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})
  .then(function(r){return r.json();}).then(function(d){
    var s=document.getElementById('mode-status');
    s.textContent=d.ok?'Saved \u2714':'Error';
    s.style.color=d.ok?'#4caf50':'#dc3545';
    setTimeout(function(){s.textContent='';},2000);
  }).catch(function(){});
}
function saveRics(){
  var excl=document.getElementById('excl-rics').value
    .replace(/[^0-9,]/g,'').replace(/,+/g,',').replace(/^,|,$/g,'');
  var body='time_ric='+encodeURIComponent(document.getElementById('time-ric').value)
          +'&call_ric='+encodeURIComponent(document.getElementById('call-ric').value)
          +'&excl_rics='+encodeURIComponent(excl);
  fetch('/api/espnow',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})
  .then(function(r){return r.json();}).then(function(d){
    var s=document.getElementById('ric-status');
    s.textContent=d.ok?'Saved \u2714':'Error';
    s.style.color=d.ok?'#4caf50':'#dc3545';
    setTimeout(function(){s.textContent='';},2500);
  }).catch(function(){});
}
</script>
</body></html>
)html";
