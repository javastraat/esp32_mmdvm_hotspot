#pragma once
#include "styles.h"
#include "navigation.h"

// ── Home page (/): display preview, clock, battery/sensors, ESP-NOW, POCSAG log ──
static const char PAGE_MAIN[] PROGMEM =
  "<!DOCTYPE html><html lang=\"en\">"
  "<head>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Ulanzi Clock</title>"
  "<style>" COMMON_CSS "</style>"
  THEME_INIT_SCRIPT
  "</head><body>"
  NAV_BAR
  NAV_LIVE_MODAL
  R"html(
<div class="container"><div class="grid">

  <div class="card" style="grid-column:1/-1">
    <h3>Display Preview</h3>
    <canvas id="led-canvas" width="320" height="80"
      style="width:100%;background:#111;border-radius:4px;image-rendering:pixelated;display:block"></canvas>
    <div style="display:flex;justify-content:center;gap:16px;margin-top:10px">
      <button onclick="pressBtn('left')"   class="btn btn-secondary" style="flex:1;max-width:100px;font-size:1.2em">&lt;</button>
      <button onclick="pressBtn('middle')" class="btn btn-secondary" style="flex:1;max-width:100px;font-size:1.2em">&#9711;</button>
      <button onclick="pressBtn('right')"  class="btn btn-secondary" style="flex:1;max-width:100px;font-size:1.2em">&gt;</button>
    </div>
  </div>

  <div class="card">
    <h3>Clock</h3>
    <div class="clock-val" id="clk">--:--:--</div>
    <div class="clock-sub" id="sync-lbl">waiting for sync...</div>
    <div class="clock-sub" id="mode-lbl" style="margin-top:4px">mode: clock</div>
  </div>

  <div class="card">
    <h3>Battery &amp; Sensors</h3>
    <div class="metric"><span class="metric-label">Battery</span><span class="metric-value" id="bat">-</span></div>
    <div class="metric"><span class="metric-label">Battery Raw</span><span class="metric-value" id="bat-raw" style="color:var(--text-muted)">-</span></div>
    <div class="metric"><span class="metric-label">Light (LDR)</span><span class="metric-value" id="ldr" style="color:var(--text-muted)">-</span></div>
    <div class="metric" id="row-temp" style="display:none"><span class="metric-label">Temperature</span><span class="metric-value" id="temp">-</span></div>
    <div class="metric" id="row-hum"  style="display:none"><span class="metric-label">Humidity</span><span class="metric-value" id="hum">-</span></div>
  </div>

  <div class="card">
    <h3>ESP-NOW</h3>
    <div class="metric"><span class="metric-label">DMR Received</span><span class="metric-value" id="dmr">-</span></div>
    <div class="metric"><span class="metric-label">POCSAG Received</span><span class="metric-value" id="poc">-</span></div>
  </div>

  <div class="card" style="grid-column:1/-1">
    <h3>Last POCSAG</h3>
    <div id="poc-log"><span style="color:#888">none yet</span></div>
  </div>

</div></div>
)html"
  "<script>" COMMON_JS NAV_LIVE_JS "</script>"
  R"html(
<script>
function pressBtn(name){
  fetch('/api/btn/'+name,{method:'POST'}).catch(function(){});
}
function fmtUp(s){
  var d=Math.floor(s/86400),h=Math.floor((s%86400)/3600),
      m=Math.floor((s%3600)/60),sc=s%60;
  return(d>0?d+'d ':'')+('0'+h).slice(-2)+':'+('0'+m).slice(-2)+':'+('0'+sc).slice(-2);
}
function rssiBar(dbm){
  var pct=Math.min(100,Math.max(0,2*(dbm+100)));
  var col=pct>60?'#28a745':pct>30?'#ffc107':'#dc3545';
  return '<span style="font-family:monospace">'+dbm+' dBm</span>'
        +'<span class="badge" style="background:'+col+';margin-left:6px">'+pct+'%</span>';
}
function poll(){
  fetch('/api/status').then(function(r){return r.json();}).then(function(d){
    document.getElementById('h1').textContent=d.hostname;
    document.getElementById('sub').textContent=d.ip;
    document.getElementById('clk').textContent=d.time_synced?d.time:'--:--:--';
    document.getElementById('sync-lbl').textContent=
      d.pocsag_synced?'synced':d.time_synced?'from RTC':'waiting for sync...';
    document.getElementById('mode-lbl').textContent='mode: '+(d.display_mode||'clock');
    if(d.sht31_available){
      document.getElementById('row-temp').style.display='';
      document.getElementById('row-hum').style.display='';
      document.getElementById('temp').textContent=d.sht31_temp.toFixed(1)+' \u00b0C';
      document.getElementById('hum').textContent=d.sht31_hum.toFixed(1)+' %';
    }
    document.getElementById('dmr').textContent=d.dmr_count;
    document.getElementById('poc').textContent=d.pocsag_count;
    var log=d.pocsag_log||[];
    var el=document.getElementById('poc-log');
    if(!log.length){
      el.innerHTML='<span style="color:#888">none yet</span>';
    } else {
      var html='<table style="width:100%;border-collapse:collapse;font-size:.85em">';
      for(var i=0;i<log.length;i++){
        html+='<tr>'
          +'<td style="color:#888;white-space:nowrap;padding:2px 8px 2px 0;vertical-align:top">RIC '+log[i].ric+'</td>'
          +'<td style="padding:2px 0;word-break:break-all">'+log[i].msg+'</td>'
          +'</tr>';
      }
      el.innerHTML=html+'</table>';
    }
    var pct=d.battery_pct,mv=d.battery_mv;
    var bc=pct>=60?'badge-success':pct>=30?'badge-warning':'badge-danger';
    document.getElementById('bat').innerHTML=
      '<span style="font-family:monospace;margin-right:6px">'+(mv/1000).toFixed(2)+' V</span>'
      +'<span class="badge '+bc+'">'+pct+'%</span>';
    document.getElementById('bat-raw').textContent=d.battery_raw+' ADC';
    document.getElementById('ldr').textContent=d.ldr_raw+' / 4095';
  }).catch(function(){});
}
function pollDisplay(){
  fetch('/api/leds').then(function(r){return r.text();}).then(function(hex){
    var canvas=document.getElementById('led-canvas');
    var ctx=canvas.getContext('2d');
    var W=32,H=8,S=10;
    ctx.fillStyle='#111';
    ctx.fillRect(0,0,canvas.width,canvas.height);
    for(var y=0;y<H;y++){
      for(var x=0;x<W;x++){
        var idx=(y%2===0)?y*W+x:(y+1)*W-1-x;
        var r=parseInt(hex.substr(idx*6,2),16);
        var g=parseInt(hex.substr(idx*6+2,2),16);
        var b=parseInt(hex.substr(idx*6+4,2),16);
        if(r>0||g>0||b>0){
          ctx.fillStyle='rgb('+r+','+g+','+b+')';
          ctx.fillRect(x*S+1,y*S+1,S-2,S-2);
        }
      }
    }
  }).catch(function(){});
  setTimeout(pollDisplay,500);
}
poll();
setInterval(poll,500);
pollDisplay();
</script>
</body></html>
)html";

// ── Fullscreen live display page (/live) ──────────────────────────────────
static const char PAGE_LIVE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Ulanzi Live</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{background:#0a0a0a;display:flex;flex-direction:column;align-items:center;justify-content:center;min-height:100vh;gap:14px}
h2{color:#444;font-family:monospace;font-size:.72em;letter-spacing:.2em;text-transform:uppercase}
canvas{display:block;width:100%;max-width:640px;border-radius:6px;background:#111;image-rendering:pixelated}
a{color:#333;font-family:monospace;font-size:.72em;text-decoration:none;letter-spacing:.1em}
a:hover{color:#00bcd4}
</style>
</head>
<body>
<h2>&#9679; Display Live</h2>
<canvas id="c" width="640" height="160"></canvas>
<a href="/">&#8592; back to dashboard</a>
<script>
var canvas=document.getElementById('c');
var ctx=canvas.getContext('2d');
var W=32,H=8,S=20;
function draw(hex){
  ctx.fillStyle='#111';
  ctx.fillRect(0,0,canvas.width,canvas.height);
  for(var y=0;y<H;y++){
    for(var x=0;x<W;x++){
      var idx=(y%2===0)?y*W+x:(y+1)*W-1-x;
      var r=parseInt(hex.substr(idx*6,2),16);
      var g=parseInt(hex.substr(idx*6+2,2),16);
      var b=parseInt(hex.substr(idx*6+4,2),16);
      if(r>0||g>0||b>0){
        ctx.fillStyle='rgb('+r+','+g+','+b+')';
        ctx.fillRect(x*S+1,y*S+1,S-2,S-2);
      }
    }
  }
}
function poll(){
  fetch('/api/leds').then(function(r){return r.text();}).then(draw).catch(function(){});
  setTimeout(poll,250);
}
poll();
</script>
</body></html>
)rawliteral";
