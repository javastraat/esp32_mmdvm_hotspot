#pragma once

static const char PAGE_MAIN[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Ulanzi Clock</title>
<style>
:root{--bg:#f0f0f0;--cont:#fff;--text:#333;--muted:#6c757d;--border:#dee2e6;--card:#f8f9fa;--bar:#333;--bar-text:#f2f2f2;--cyan:#0097a7;--green:#388e3c;--amber:#e65100;--red:#c62828;--dim:#999}
[data-theme=dark]{--bg:#1a1a1a;--cont:#2d2d2d;--text:#fff;--muted:#adb5bd;--border:#555;--card:#3a3a3a;--bar:#111;--bar-text:#f2f2f2;--cyan:#00bcd4;--green:#4caf50;--amber:#ffb300;--red:#f44336;--dim:#888}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:Arial,sans-serif;background:var(--bg);color:var(--text);transition:background .3s,color .3s;padding-bottom:30px}
.bar{background:var(--bar);color:var(--bar-text);padding:0 20px;height:50px;display:flex;align-items:center;justify-content:space-between;position:sticky;top:0;z-index:100;box-shadow:0 2px 5px rgba(0,0,0,.3)}
.bar-left{display:flex;align-items:baseline;gap:10px}
.bar h1{font-size:1.1em;font-weight:bold;color:var(--bar-text)}
.bar .sub{font-size:.75em;color:#aaa}
.theme-btn{background:none;border:none;font-size:1.2em;cursor:pointer;padding:6px 10px;border-radius:50%;color:var(--bar-text);transition:background .2s;line-height:1}
.theme-btn:hover{background:rgba(255,255,255,.15)}
.cont{max-width:700px;margin:20px auto;padding:0 16px}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.card{background:var(--card);border:1px solid var(--border);border-radius:6px;padding:12px 15px;transition:background .3s,border-color .3s}
.lbl{color:var(--muted);font-size:.72em;text-transform:uppercase;letter-spacing:.05em;margin-bottom:4px}
.val{font-size:1.05em;color:var(--text);word-break:break-all;font-family:monospace}
.val.big{font-size:1.5em;letter-spacing:.06em}
.val.cyan{color:var(--cyan)}
.val.green{color:var(--green)}
.val.amber{color:var(--amber)}
.val.red{color:var(--red)}
.val.dim{color:var(--dim)}
.full{grid-column:1/-1}
.bright-row{display:flex;align-items:center;gap:12px;margin-top:8px}
.switch{position:relative;display:inline-block;width:48px;height:26px;flex-shrink:0}
.switch input{opacity:0;width:0;height:0}
.sw-sl{position:absolute;inset:0;background:#aaa;border-radius:26px;cursor:pointer;transition:.3s}
.sw-sl:before{content:'';position:absolute;width:20px;height:20px;left:3px;top:3px;background:#fff;border-radius:50%;transition:.3s;box-shadow:0 1px 3px rgba(0,0,0,.3)}
.switch input:checked+.sw-sl{background:var(--cyan)}
.switch input:checked+.sw-sl:before{transform:translateX(22px)}
.sw-lbl{color:var(--muted);font-size:.85em;white-space:nowrap;font-family:Arial,sans-serif}
input[type=range]{flex:1;accent-color:var(--cyan);cursor:pointer}
input[type=range]:disabled{opacity:.35;cursor:default}
.bright-val{color:var(--muted);font-size:.85em;min-width:28px;text-align:right;font-family:monospace}
@media(max-width:440px){.grid{grid-template-columns:1fr}}
</style>
<script>
(function(){
  var t=localStorage.getItem('theme')||'dark';
  document.documentElement.setAttribute('data-theme',t);
})();
</script>
</head>
<body>
<div class="bar">
  <div class="bar-left">
    <h1 id="h1">Ulanzi Clock</h1>
    <span class="sub" id="sub">connecting...</span>
  </div>
  <button class="theme-btn" id="theme-btn" onclick="toggleTheme()" title="Toggle dark/light">&#127773;</button>
</div>
<div class="cont">
<div class="grid">
  <div class="card"><div class="lbl">Role</div><div class="val" id="role">-</div></div>
  <div class="card"><div class="lbl">Uptime</div><div class="val" id="uptime">-</div></div>
  <div class="card"><div class="lbl">IP Address</div><div class="val green" id="ip">-</div></div>
  <div class="card"><div class="lbl">WiFi Channel</div><div class="val" id="ch">-</div></div>
  <div class="card"><div class="lbl">Battery</div><div class="val" id="bat">-</div></div>
  <div class="card"><div class="lbl">Light (LDR)</div><div class="val dim" id="ldr">-</div></div>
  <div class="card full"><div class="lbl">Time</div><div class="val big cyan" id="clk">-</div></div>
  <div class="card"><div class="lbl" id="lbl-dmr">DMR</div><div class="val" id="dmr">-</div></div>
  <div class="card"><div class="lbl" id="lbl-poc">POCSAG</div><div class="val" id="poc">-</div></div>
  <div class="card full" id="card-msg">
    <div class="lbl">Last POCSAG</div>
    <div class="val amber" id="msg">-</div>
  </div>
  <div class="card full">
    <div class="lbl">Brightness</div>
    <div class="bright-row">
      <label class="switch">
        <input type="checkbox" id="tog-auto" onchange="onAutoToggle()">
        <span class="sw-sl"></span>
      </label>
      <span class="sw-lbl" id="tog-lbl">Auto</span>
      <input type="range" id="sld-bright" min="1" max="255" value="50" disabled
             oninput="document.getElementById('bright-val').textContent=this.value"
             onchange="onSliderChange()">
      <span class="bright-val" id="bright-val">50</span>
    </div>
  </div>
</div>
</div>

<script>
function toggleTheme(){
  var t=document.documentElement.getAttribute('data-theme')==='dark'?'light':'dark';
  document.documentElement.setAttribute('data-theme',t);
  localStorage.setItem('theme',t);
  document.getElementById('theme-btn').innerHTML=t==='dark'?'&#127773;':'&#127769;';
}
document.addEventListener('DOMContentLoaded',function(){
  var t=document.documentElement.getAttribute('data-theme');
  document.getElementById('theme-btn').innerHTML=t==='dark'?'&#127773;':'&#127769;';
});
function fmtUp(s){
  var d=Math.floor(s/86400),h=Math.floor((s%86400)/3600),
      m=Math.floor((s%3600)/60),sc=s%60;
  return (d>0?d+'d ':'')+('0'+h).slice(-2)+':'+('0'+m).slice(-2)+':'+('0'+sc).slice(-2);
}
function poll(){
  fetch('/api/status').then(function(r){return r.json();}).then(function(d){
    document.getElementById('h1').textContent=d.hostname;
    document.getElementById('sub').textContent=d.ip;
    document.getElementById('role').textContent=d.role;
    document.getElementById('uptime').textContent=fmtUp(d.uptime);
    document.getElementById('ip').textContent=d.ip;
    document.getElementById('ch').textContent='ch '+d.channel;
    document.getElementById('clk').textContent=d.time_synced?d.time:'not synced';
    var isSender=(d.role==='SENDER');
    document.getElementById('lbl-dmr').textContent=isSender?'DMR Sent':'DMR Recv';
    document.getElementById('lbl-poc').textContent=isSender?'POCSAG Sent':'POCSAG Recv';
    document.getElementById('dmr').textContent=d.dmr_count;
    document.getElementById('poc').textContent=d.pocsag_count;
    var msg=d.last_pocsag||'';
    document.getElementById('card-msg').style.display=(!isSender||msg)?'':'none';
    document.getElementById('msg').textContent=msg||'(none)';
    var batEl=document.getElementById('bat');
    var pct=d.battery_pct,mv=d.battery_mv;
    batEl.textContent=(mv/1000).toFixed(2)+' V \u00B7 '+pct+'%';
    batEl.className='val '+(pct>=60?'green':pct>=30?'amber':'red');
    document.getElementById('ldr').textContent=d.ldr_raw+' / 4095';
    document.getElementById('tog-auto').checked=d.auto_brightness;
    document.getElementById('sld-bright').disabled=d.auto_brightness;
    document.getElementById('tog-lbl').textContent=d.auto_brightness?'Auto':'Manual';
    if(d.auto_brightness){
      document.getElementById('sld-bright').value=d.brightness;
      document.getElementById('bright-val').textContent=d.brightness;
    }
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
  document.getElementById('bright-val').textContent=level;
  postBright(false,level);
}
poll();
setInterval(poll,2000);
</script>
</body>
</html>
)rawliteral";
