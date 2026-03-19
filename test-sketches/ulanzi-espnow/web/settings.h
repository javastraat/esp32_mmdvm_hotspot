#pragma once
#include "styles.h"
#include "navigation.h"

// ── Settings page (/settings): buzzer, device name, system ────────────────
static const char PAGE_SETTINGS[] PROGMEM =
  "<!DOCTYPE html><html lang=\"en\">"
  "<head>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Ulanzi Settings</title>"
  "<style>" COMMON_CSS "</style>"
  THEME_INIT_SCRIPT
  "</head><body>"
  NAV_BAR
  NAV_LIVE_MODAL
  R"html(
<div class="container"><div class="grid">

  <!-- Buzzer -->
  <div class="card">
    <h3>Buzzer</h3>
    <div style="padding:6px 0;border-bottom:1px solid var(--border-color)">
      <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:8px">
        <span class="metric-label">Boot Sound</span>
        <label class="switch">
          <input type="checkbox" id="tog-bz-boot" onchange="onBuzzerChange(this.checked?'boot':null)">
          <span class="slider"></span>
        </label>
      </div>
      <div class="bright-bot">
        <input type="range" id="sld-bz-boot" min="1" max="255" value="80"
               oninput="document.getElementById('bz-boot-num').textContent=this.value"
               onchange="onBuzzerChange('boot')">
        <span class="bright-num" id="bz-boot-num">80</span>
        <button onclick="testBuzzer('boot')" class="btn btn-secondary" style="flex-shrink:0">Test</button>
      </div>
    </div>
    <div style="padding:6px 0;border-bottom:1px solid var(--border-color)">
      <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:8px">
        <span class="metric-label">POCSAG Receive</span>
        <label class="switch">
          <input type="checkbox" id="tog-bz-poc" onchange="onBuzzerChange(this.checked?'pocsag':null)">
          <span class="slider"></span>
        </label>
      </div>
      <div class="bright-bot">
        <input type="range" id="sld-bz-poc" min="1" max="255" value="80"
               oninput="document.getElementById('bz-poc-num').textContent=this.value"
               onchange="onBuzzerChange('pocsag')">
        <span class="bright-num" id="bz-poc-num">80</span>
        <button onclick="testBuzzer('pocsag')" class="btn btn-secondary" style="flex-shrink:0">Test</button>
      </div>
    </div>
    <div style="padding:6px 0">
      <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:8px">
        <span class="metric-label">Button Click</span>
        <label class="switch">
          <input type="checkbox" id="tog-bz-clk" onchange="onBuzzerChange(this.checked?'click':null)">
          <span class="slider"></span>
        </label>
      </div>
      <div class="bright-bot">
        <input type="range" id="sld-bz-clk" min="1" max="255" value="60"
               oninput="document.getElementById('bz-clk-num').textContent=this.value"
               onchange="onBuzzerChange('click')">
        <span class="bright-num" id="bz-clk-num">60</span>
        <button onclick="testBuzzer('click')" class="btn btn-secondary" style="flex-shrink:0">Test</button>
      </div>
    </div>
  </div>

  <!-- Device Name -->
  <div class="card">
    <h3>Device Name</h3>
    <div style="margin-bottom:12px">
      <div style="font-size:.82em;color:var(--text-muted);margin-bottom:6px">Boot screen name (max 8 chars)</div>
      <div style="display:flex;align-items:center;gap:8px">
        <input type="text" id="boot-name" maxlength="8"
               style="flex:1;min-width:0;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:5px 8px;font-size:1em;text-transform:uppercase;letter-spacing:.1em"
               oninput="this.value=this.value.toUpperCase()">
        <button onclick="saveBootName()" class="btn btn-info">Save</button>
      </div>
      <div id="bootname-status" style="font-size:.78em;color:#4caf50;margin-top:4px;min-height:1em"></div>
    </div>
    <div style="margin-bottom:12px">
      <div style="font-size:.82em;color:var(--text-muted);margin-bottom:6px">Hostname — web &amp; OTA (<span id="mdns-preview">ulanzi</span>.local)</div>
      <div style="display:flex;align-items:center;gap:8px">
        <input type="text" id="mdns-name" maxlength="31"
               style="flex:1;min-width:0;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:5px 8px;font-size:1em"
               oninput="this.value=this.value.toLowerCase().replace(/[^a-z0-9-]/g,'');document.getElementById('mdns-preview').textContent=this.value||'ulanzi'">
        <button onclick="saveMdnsName()" class="btn btn-info">Save</button>
      </div>
      <div id="mdnsname-status" style="font-size:.78em;color:#4caf50;margin-top:4px;min-height:1em"></div>
    </div>
  </div>

  <!-- System Actions -->
  <div class="card">
    <h3>System</h3>
    <div style="display:flex;flex-direction:column;gap:10px">
      <div class="metric" style="border-bottom:none">
        <div>
          <span class="metric-label">Verbose Logging</span>
          <div style="font-size:.75em;color:var(--text-muted);margin-top:2px">Enables [GIF], [SHT31] and other debug messages in the Serial page.</div>
        </div>
        <label class="switch" style="margin-left:12px;flex-shrink:0">
          <input type="checkbox" id="debug-log-toggle" onchange="setDebugLog(this.checked)">
          <span class="slider"></span>
        </label>
      </div>
      <div>
        <button onclick="doClearRtc()" class="btn btn-warning" style="width:100%">Clear RTC &amp; Time Sync</button>
        <div id="rtc-status" style="font-size:.78em;margin-top:3px;min-height:1em;color:var(--text-muted)">Clears the DS1307 and resets sync flags. Scanner animation runs until the next time beacon.</div>
      </div>
      <div>
        <button onclick="doReboot()" class="btn btn-danger" style="width:100%">Reboot Device</button>
        <div id="reboot-status" style="font-size:.78em;color:#aaa;margin-top:3px;min-height:1em"></div>
      </div>
      <div>
        <button onclick="doFactoryReset()" class="btn btn-danger" style="width:100%;background:#7b0000">Factory Reset</button>
        <div style="font-size:.75em;color:var(--text-muted);margin-top:3px">Clears all saved settings and reboots.</div>
      </div>
    </div>
  </div>

</div></div>
)html"
  "<script>" COMMON_JS NAV_LIVE_JS "</script>"
  R"html(
<script>
function onBuzzerChange(testType){
  fetch('/api/buzzer',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'boot_en='+(document.getElementById('tog-bz-boot').checked?1:0)
        +'&boot_vol='+document.getElementById('sld-bz-boot').value
        +'&pocsag_en='+(document.getElementById('tog-bz-poc').checked?1:0)
        +'&pocsag_vol='+document.getElementById('sld-bz-poc').value
        +'&click_en='+(document.getElementById('tog-bz-clk').checked?1:0)
        +'&click_vol='+document.getElementById('sld-bz-clk').value
  }).then(function(){
    if(testType)testBuzzer(testType);
  }).catch(function(){});
}
function testBuzzer(type){
  var vol=type==='boot'?document.getElementById('sld-bz-boot').value
         :type==='pocsag'?document.getElementById('sld-bz-poc').value
         :document.getElementById('sld-bz-clk').value;
  fetch('/api/buzzer/test',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'type='+type+'&vol='+vol
  }).catch(function(){});
}
function doClearRtc(){
  showConfirm('Clear RTC and time sync? Scanner animation will run until the next time beacon.',function(){
    var s=document.getElementById('rtc-status');
    fetch('/api/rtc/clear',{method:'POST'}).then(function(){
      s.style.color='#4caf50';
      s.textContent='RTC cleared \u2714 Scanner running until next time beacon.';
      setTimeout(function(){
        s.style.color='var(--text-muted)';
        s.textContent='Clears the DS1307 and resets sync flags. Scanner animation runs until the next time beacon.';
      },5000);
    }).catch(function(){
      s.style.color='#f44336';
      s.textContent='Error clearing RTC.';
    });
  });
}
function doReboot(){
  showConfirm('Reboot device?',function(){
    var s=document.getElementById('reboot-status');
    s.textContent='Rebooting\u2026';
    fetch('/api/reboot',{method:'POST'}).catch(function(){});
    setTimeout(function(){s.textContent='Reconnecting\u2026';},1500);
    setTimeout(function(){location.reload();},6000);
  });
}
function doFactoryReset(){
  showConfirm('Factory reset?\n\nThis will erase ALL saved settings and reboot.',function(){
    showConfirm('Are you sure? This cannot be undone.',function(){
      fetch('/api/factory-reset',{method:'POST'}).catch(function(){});
      document.body.innerHTML='<div style="display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:1.2em">Factory reset \u2014 rebooting\u2026</div>';
      setTimeout(function(){location.href='/';},8000);
    });
  });
}
function setDebugLog(val){
  fetch('/api/debug',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({debug:val})}).catch(function(){});
}
function saveMdnsName(){
  var v=document.getElementById('mdns-name').value.trim().toLowerCase().replace(/[^a-z0-9-]/g,'');
  if(v.length===0||v.length>31){
    document.getElementById('mdnsname-status').textContent='1\u201331 chars (a-z, 0-9, -)';
    document.getElementById('mdnsname-status').style.color='#f44';
    return;
  }
  fetch('/api/mdnsname',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'name='+encodeURIComponent(v)})
  .then(function(r){return r.json();}).then(function(d){
    var s=document.getElementById('mdnsname-status');
    if(d.ok){
      document.getElementById('mdns-preview').textContent=v;
      showConfirm('Saved. Reboot now for '+v+'.local to take effect?',
        function(){fetch('/api/reboot',{method:'POST'}).catch(function(){});},
        function(){s.textContent='Saved \u2014 reboot required';s.style.color='#ffc107';setTimeout(function(){s.textContent='';},4000);}
      );
    } else {
      s.textContent='Error: '+(d.error||'?');s.style.color='#f44';
    }
  }).catch(function(){});
}
function saveBootName(){
  var v=document.getElementById('boot-name').value.trim().toUpperCase();
  if(v.length===0||v.length>8){
    document.getElementById('bootname-status').textContent='1\u20138 characters required.';
    document.getElementById('bootname-status').style.color='#f44';
    return;
  }
  fetch('/api/bootname',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'name='+encodeURIComponent(v)})
  .then(function(r){return r.json();}).then(function(d){
    var s=document.getElementById('bootname-status');
    s.textContent=d.ok?'Saved.':'Error: '+(d.error||'?');
    s.style.color=d.ok?'#4caf50':'#f44';
    setTimeout(function(){s.textContent='';},2000);
  }).catch(function(){});
}
(function init(){
  fetch('/api/status').then(function(r){return r.json();}).then(function(d){
    document.getElementById('h1').textContent=d.hostname;
    document.getElementById('sub').textContent=d.ip;
    document.getElementById('tog-bz-boot').checked=d.buzzer_boot_en;
    document.getElementById('sld-bz-boot').value=d.buzzer_boot_vol;
    document.getElementById('bz-boot-num').textContent=d.buzzer_boot_vol;
    document.getElementById('tog-bz-poc').checked=d.buzzer_pocsag_en;
    document.getElementById('sld-bz-poc').value=d.buzzer_pocsag_vol;
    document.getElementById('bz-poc-num').textContent=d.buzzer_pocsag_vol;
    document.getElementById('tog-bz-clk').checked=d.buzzer_click_en;
    document.getElementById('sld-bz-clk').value=d.buzzer_click_vol;
    document.getElementById('bz-clk-num').textContent=d.buzzer_click_vol;
  }).catch(function(){});
  fetch('/api/bootname').then(function(r){return r.json();}).then(function(d){
    if(d.name)document.getElementById('boot-name').value=d.name;
  }).catch(function(){});
  fetch('/api/debug').then(function(r){return r.json();}).then(function(d){
    document.getElementById('debug-log-toggle').checked=d.debug;
  }).catch(function(){});
  fetch('/api/mdnsname').then(function(r){return r.json();}).then(function(d){
    if(d.name){
      document.getElementById('mdns-name').value=d.name;
      document.getElementById('mdns-preview').textContent=d.name;
    }
  }).catch(function(){});
})();
</script>
</body></html>
)html";
