#pragma once

static const char PAGE_MAIN[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Ulanzi Clock</title>
<style>
/* CSS Variables — exact match to main project styles.h */
:root{--bg-color:#f0f0f0;--container-bg:white;--text-color:#333;--text-primary:#333;--text-muted:#6c757d;--border-color:#dee2e6;--card-bg:#f8f9fa;--bg-secondary:#f8f9fa;--topnav-bg:#333;--topnav-text:#f2f2f2;--topnav-hover:#ddd;--topnav-hover-text:black}
[data-theme='dark']{--bg-color:#1a1a1a;--container-bg:#2d2d2d;--text-color:#ffffff;--text-primary:#ffffff;--text-muted:#adb5bd;--border-color:#555;--card-bg:#3a3a3a;--bg-secondary:#3a3a3a;--topnav-bg:#000;--topnav-text:#f2f2f2;--topnav-hover:#444;--topnav-hover-text:#ffffff}
/* Base */
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:Arial,sans-serif;background:var(--bg-color);color:var(--text-color);transition:background-color .3s,color .3s;padding-top:60px;padding-bottom:30px}
p,div,span,strong,label{color:var(--text-color)}
/* Navbar */
.navbar{position:fixed;top:0;left:0;right:0;background:var(--topnav-bg);border-bottom:1px solid var(--border-color);box-shadow:0 2px 5px rgba(0,0,0,.3);z-index:1000;display:flex;align-items:center;padding:0 20px;height:60px}
.nav-brand{font-size:1.2em;font-weight:bold;color:var(--topnav-text);margin-right:14px}
.nav-sub{font-size:.8em;color:#aaa;font-family:monospace}
.theme-toggle{margin-left:auto;cursor:pointer;background:var(--topnav-hover);border:none;padding:10px 15px;border-radius:50%;font-size:1.2em;color:var(--topnav-text)}
.theme-toggle:hover{background:#007bff;color:white}
/* Container */
.container{max-width:1100px;margin:20px auto;background:var(--container-bg);padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,.1)}
/* Grid — all cards equal, same as main project */
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:15px;margin:10px 0}
/* Cards */
.card{background:var(--card-bg);padding:15px;border-radius:6px;border:1px solid var(--border-color)}
.card h3{margin:0 0 12px 0;font-size:.75em;font-weight:bold;text-transform:uppercase;letter-spacing:.06em;color:var(--text-muted)}
/* Metric rows */
.metric{display:flex;justify-content:space-between;align-items:center;padding:7px 0;border-bottom:1px solid var(--border-color)}
.metric:last-child{border-bottom:none}
.metric-label{font-weight:bold;color:var(--text-muted);font-size:.88em}
.metric-value{color:var(--text-color);font-family:monospace;font-size:.9em;text-align:right}
/* Badge pills */
.badge{display:inline-block;font-size:.75em;font-weight:bold;padding:2px 10px;border-radius:12px;color:#fff}
.badge-success{background:#28a745}
.badge-warning{background:#ffc107;color:#212529}
.badge-danger{background:#dc3545}
/* Clock */
.clock-val{font-size:2.2em;font-family:monospace;letter-spacing:.08em;color:#00bcd4;margin:8px 0 4px}
.clock-sub{font-size:.8em;color:var(--text-muted)}
/* POCSAG message */
.pocsag-msg{font-family:monospace;font-size:1em;color:#ffb300;word-break:break-all;padding:6px 0;min-height:2em}
/* Toggle switch — exact from styles.h */
.switch{position:relative;display:inline-block;width:60px;height:34px;flex-shrink:0}
.switch input{opacity:0;width:0;height:0}
.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background-color:#ccc;transition:.4s;border-radius:34px;box-shadow:0 0 2px #000}
.slider:before{position:absolute;content:'';height:26px;width:26px;left:4px;bottom:4px;background-color:white;transition:.4s;border-radius:50%}
input:checked+.slider{background-color:#4CAF50}
input:not(:checked)+.slider{background-color:#f44336}
input:checked+.slider:before{transform:translateX(26px)}
/* Brightness card */
.bright-top{display:flex;align-items:center;gap:12px;margin-bottom:12px}
.bright-lbl{color:var(--text-muted);font-size:.88em;white-space:nowrap}
.bright-bot{display:flex;align-items:center;gap:10px}
input[type=range]{flex:1;accent-color:#00bcd4;cursor:pointer;min-width:0}
input[type=range]:disabled{opacity:.35;cursor:default}
.bright-num{color:var(--text-muted);font-size:.88em;min-width:30px;text-align:right;font-family:monospace;flex-shrink:0}
</style>
<script>
(function(){
  var t=localStorage.getItem('theme')||'dark';
  document.documentElement.setAttribute('data-theme',t);
})();
</script>
</head>
<body>

<div class="navbar">
  <span class="nav-brand" id="h1">Ulanzi Clock</span>
  <span class="nav-sub" id="sub"></span>
  <button class="theme-toggle" id="theme-btn" onclick="toggleTheme()">&#127769;</button>
</div>

<div class="container">
<div class="grid">

  <!-- 1. Device -->
  <div class="card">
    <h3>Device</h3>
    <div class="metric"><span class="metric-label">Hostname</span><span class="metric-value" id="hostname">-</span></div>
    <div class="metric"><span class="metric-label">IP Address</span><span class="metric-value" id="ip" style="color:#28a745">-</span></div>
    <div class="metric"><span class="metric-label">Uptime</span><span class="metric-value" id="uptime">-</span></div>
    <div class="metric"><span class="metric-label">Free Heap</span><span class="metric-value" id="heap">-</span></div>
  </div>

  <!-- 2. WiFi -->
  <div class="card">
    <h3>WiFi</h3>
    <div class="metric"><span class="metric-label">SSID</span><span class="metric-value" id="ssid">-</span></div>
    <div class="metric"><span class="metric-label">Channel</span><span class="metric-value" id="ch">-</span></div>
    <div class="metric"><span class="metric-label">RSSI</span><span class="metric-value" id="rssi">-</span></div>
    <div class="metric"><span class="metric-label">MAC Address</span><span class="metric-value" id="mac">-</span></div>
  </div>

  <!-- 3. Hardware -->
  <div class="card">
    <h3>Hardware</h3>
    <div class="metric"><span class="metric-label">Platform</span><span class="metric-value">ESP32-WROOM-32D</span></div>
    <div class="metric"><span class="metric-label">CPU</span><span class="metric-value">Xtensa LX6 240 MHz</span></div>
    <div class="metric"><span class="metric-label">Flash</span><span class="metric-value">8 MB</span></div>
    <div class="metric"><span class="metric-label">LED Matrix</span><span class="metric-value">32&#xd7;8 WS2812B</span></div>
    <div class="metric"><span class="metric-label">LDR</span><span class="metric-value">GL5516 GPIO35</span></div>
    <div class="metric"><span class="metric-label">Battery</span><span class="metric-value">4400 mAh LiPo</span></div>
  </div>

  <!-- 4. Battery & Sensors -->
  <div class="card">
    <h3>Battery &amp; Sensors</h3>
    <div class="metric"><span class="metric-label">Battery</span><span class="metric-value" id="bat">-</span></div>
    <div class="metric"><span class="metric-label">Battery Raw</span><span class="metric-value" id="bat-raw" style="color:var(--text-muted)">-</span></div>
    <div class="metric"><span class="metric-label">Light (LDR)</span><span class="metric-value" id="ldr" style="color:var(--text-muted)">-</span></div>
    <div class="metric" id="row-temp" style="display:none"><span class="metric-label">Temperature</span><span class="metric-value" id="temp">-</span></div>
    <div class="metric" id="row-hum"  style="display:none"><span class="metric-label">Humidity</span><span class="metric-value" id="hum">-</span></div>
  </div>

  <!-- 5. Clock -->
  <div class="card">
    <h3>Clock</h3>
    <div class="clock-val" id="clk">--:--:--</div>
    <div class="clock-sub" id="sync-lbl">waiting for sync...</div>
    <div class="clock-sub" id="mode-lbl" style="margin-top:4px">mode: clock</div>
  </div>

  <!-- 6. ESP-NOW -->
  <div class="card">
    <h3>ESP-NOW</h3>
    <div class="metric"><span class="metric-label">DMR Received</span><span class="metric-value" id="dmr">-</span></div>
    <div class="metric"><span class="metric-label">POCSAG Received</span><span class="metric-value" id="poc">-</span></div>
  </div>

  <!-- 7. Last POCSAG -->
  <div class="card" id="card-msg">
    <h3>Last POCSAG</h3>
    <div id="poc-log"><span style="color:#888">none yet</span></div>
  </div>

  <!-- 8. Brightness -->
  <div class="card">
    <h3>Brightness</h3>
    <div class="bright-top">
      <label class="switch">
        <input type="checkbox" id="tog-auto" onchange="onAutoToggle()">
        <span class="slider"></span>
      </label>
      <span class="bright-lbl" id="tog-lbl">Auto</span>
    </div>
    <div class="bright-bot">
      <input type="range" id="sld-bright" min="1" max="255" value="50" disabled
             oninput="document.getElementById('bright-num').textContent=this.value"
             onchange="onSliderChange()">
      <span class="bright-num" id="bright-num">50</span>
    </div>
  </div>

  <!-- 9. Buzzer -->
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
      </div>
    </div>
  </div>

  <!-- 10. Display Rotation -->
  <div class="card">
    <h3>Display Rotation</h3>
    <div style="padding:6px 0;border-bottom:1px solid var(--border-color)">
      <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:8px">
        <span class="metric-label">Auto-cycle screens</span>
        <label class="switch">
          <input type="checkbox" id="tog-rot" onchange="onRotateChange()">
          <span class="slider"></span>
        </label>
      </div>
      <div class="bright-bot">
        <span class="metric-label" style="white-space:nowrap;flex-shrink:0">Speed</span>
        <input type="range" id="sld-rot" min="1" max="60" value="5"
               oninput="document.getElementById('rot-num').textContent=this.value+'s'"
               onchange="onRotateChange()">
        <span class="bright-num" id="rot-num">5s</span>
      </div>
    </div>
    <div class="metric" style="border-bottom:none;padding-top:8px">
      <span class="metric-label" style="font-size:.8em;color:var(--text-muted)">clock &#8594; temp &#8594; humidity &#8594; clock</span>
    </div>
  </div>

  <!-- 11. System -->
  <div class="card">
    <h3>System</h3>
    <div class="metric">
      <span class="metric-label">Reboot device</span>
      <button onclick="doReboot()" style="background:#dc3545;color:#fff;border:none;padding:6px 16px;border-radius:4px;cursor:pointer;font-size:.85em;font-weight:bold">Reboot</button>
    </div>
  </div>

</div>
</div>

<script>
function toggleTheme(){
  var t=document.documentElement.getAttribute('data-theme')==='dark'?'light':'dark';
  document.documentElement.setAttribute('data-theme',t);
  localStorage.setItem('theme',t);
  document.getElementById('theme-btn').innerHTML=t==='dark'?'&#127769;':'&#9728;&#65039;';
}
document.addEventListener('DOMContentLoaded',function(){
  var t=document.documentElement.getAttribute('data-theme');
  document.getElementById('theme-btn').innerHTML=t==='dark'?'&#127769;':'&#9728;&#65039;';
});
function fmtUp(s){
  var d=Math.floor(s/86400),h=Math.floor((s%86400)/3600),
      m=Math.floor((s%3600)/60),sc=s%60;
  return (d>0?d+'d ':'')+('0'+h).slice(-2)+':'+('0'+m).slice(-2)+':'+('0'+sc).slice(-2);
}
function rssiBar(dbm){
  var pct=Math.min(100,Math.max(0,2*(dbm+100)));
  var col=pct>60?'#28a745':pct>30?'#ffc107':'#dc3545';
  return '<span style="font-family:monospace;">'+dbm+' dBm</span>'
        +'<span class="badge" style="background:'+col+';margin-left:6px;">'+pct+'%</span>';
}
function poll(){
  fetch('/api/status').then(function(r){return r.json();}).then(function(d){
    // Navbar
    document.getElementById('h1').textContent=d.hostname;
    document.getElementById('sub').textContent=d.ip;
    // Device card
    document.getElementById('hostname').textContent=d.hostname;
    document.getElementById('ip').textContent=d.ip;
    document.getElementById('uptime').textContent=fmtUp(d.uptime);
    document.getElementById('heap').textContent=d.free_heap?Math.round(d.free_heap/1024)+' KB':'-';
    // WiFi card
    document.getElementById('ssid').textContent=d.ssid||'-';
    document.getElementById('ch').textContent='ch '+d.channel;
    document.getElementById('rssi').innerHTML=d.rssi?rssiBar(d.rssi):'-';
    document.getElementById('mac').textContent=d.mac||'-';
    // Clock card
    document.getElementById('clk').textContent=d.time_synced?d.time:'--:--:--';
    document.getElementById('sync-lbl').textContent=d.pocsag_synced?'synced':d.time_synced?'from RTC':'waiting for sync...';
    document.getElementById('mode-lbl').textContent='mode: '+(d.display_mode||'clock');
    // Display rotation card
    document.getElementById('tog-rot').checked=d.rotate_enabled;
    var ri=d.rotate_interval||5;
    document.getElementById('sld-rot').value=ri;
    document.getElementById('rot-num').textContent=ri+'s';
    // Sensors (SHT31)
    if(d.sht31_available){
      document.getElementById('row-temp').style.display='';
      document.getElementById('row-hum').style.display='';
      document.getElementById('temp').textContent=d.sht31_temp.toFixed(1)+' \u00b0C';
      document.getElementById('hum').textContent=d.sht31_hum.toFixed(1)+' %';
    }
    // ESP-NOW card
    document.getElementById('dmr').textContent=d.dmr_count;
    document.getElementById('poc').textContent=d.pocsag_count;
    // Last POCSAG log
    var log=d.pocsag_log||[];
    var el=document.getElementById('poc-log');
    if(!log.length){el.innerHTML='<span style="color:#888">none yet</span>';}
    else{
      var html='<table style="width:100%;border-collapse:collapse;font-size:0.85em">';
      for(var i=0;i<log.length;i++){
        html+='<tr><td style="color:#888;white-space:nowrap;padding:2px 8px 2px 0">RIC '+log[i].ric+'</td>'
             +'<td style="padding:2px 0">'+log[i].msg+'</td></tr>';
      }
      html+='</table>';
      el.innerHTML=html;
    }
    // Battery card
    var pct=d.battery_pct,mv=d.battery_mv;
    var badgeCls=pct>=60?'badge badge-success':pct>=30?'badge badge-warning':'badge badge-danger';
    document.getElementById('bat').innerHTML=
      '<span style="font-family:monospace;margin-right:6px;">'+(mv/1000).toFixed(2)+' V</span>'+
      '<span class="'+badgeCls+'">'+pct+'%</span>';
    document.getElementById('bat-raw').textContent=d.battery_raw+' ADC';
    document.getElementById('ldr').textContent=d.ldr_raw+' / 4095';
    // Brightness card
    document.getElementById('tog-auto').checked=d.auto_brightness;
    document.getElementById('sld-bright').disabled=d.auto_brightness;
    document.getElementById('tog-lbl').textContent=d.auto_brightness?'Auto':'Manual';
    if(d.auto_brightness){
      document.getElementById('sld-bright').value=d.brightness;
      document.getElementById('bright-num').textContent=d.brightness;
    }
    // Buzzer card
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
}
function postBright(isAuto,level){
  fetch('/api/brightness',{
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'auto='+(isAuto?1:0)+'&level='+level
  }).catch(function(){});
}
function onAutoToggle(){
  var isAuto=document.getElementById('tog-auto').checked;
  var level=document.getElementById('sld-bright').value;
  document.getElementById('sld-bright').disabled=isAuto;
  document.getElementById('tog-lbl').textContent=isAuto?'Auto':'Manual';
  postBright(isAuto,level);
}
function onSliderChange(){
  var level=document.getElementById('sld-bright').value;
  document.getElementById('bright-num').textContent=level;
  postBright(false,level);
}
function onBuzzerChange(type){
  var vol=type==='boot'?document.getElementById('sld-bz-boot').value
         :type==='pocsag'?document.getElementById('sld-bz-poc').value
         :type==='click'?document.getElementById('sld-bz-clk').value:null;
  fetch('/api/buzzer',{
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'boot_en='+(document.getElementById('tog-bz-boot').checked?1:0)
        +'&boot_vol='+document.getElementById('sld-bz-boot').value
        +'&pocsag_en='+(document.getElementById('tog-bz-poc').checked?1:0)
        +'&pocsag_vol='+document.getElementById('sld-bz-poc').value
        +'&click_en='+(document.getElementById('tog-bz-clk').checked?1:0)
        +'&click_vol='+document.getElementById('sld-bz-clk').value
  }).then(function(){
    if(!type||!vol)return;
    fetch('/api/buzzer/test',{
      method:'POST',
      headers:{'Content-Type':'application/x-www-form-urlencoded'},
      body:'type='+type+'&vol='+vol
    }).catch(function(){});
  }).catch(function(){});
}
function onRotateChange(){
  fetch('/api/rotate',{
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'enabled='+(document.getElementById('tog-rot').checked?1:0)
        +'&interval='+document.getElementById('sld-rot').value
  }).catch(function(){});
}
function doReboot(){
  if(!confirm('Reboot Ulanzi?'))return;
  fetch('/api/reboot',{method:'POST'}).catch(function(){});
  setTimeout(function(){location.reload();},5000);
}
poll();
setInterval(poll,2000);
</script>
</body>
</html>
)rawliteral";
