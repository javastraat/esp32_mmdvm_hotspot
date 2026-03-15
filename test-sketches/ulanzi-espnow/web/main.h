#pragma once

static const char PAGE_MAIN[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Ulanzi Clock</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:monospace;background:#111;color:#ddd;padding:18px;font-size:14px}
h1{color:#00bcd4;font-size:1.35em;margin-bottom:3px}
.sub{color:#666;font-size:.8em;margin-bottom:18px}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:12px}
.card{background:#1e1e1e;border:1px solid #2a2a2a;border-radius:6px;padding:12px 15px}
.lbl{color:#666;font-size:.72em;text-transform:uppercase;letter-spacing:.05em;margin-bottom:4px}
.val{font-size:1.05em;word-break:break-all}
.val.green{color:#4caf50}
.val.cyan{color:#00bcd4;font-size:1.25em}
.val.amber{color:#ffb300}
.val.dim{color:#888}
.val.red{color:#f44336}
.full{grid-column:1/-1}
.bright-row{display:flex;align-items:center;gap:12px;margin-top:8px}
.tog{position:relative;display:inline-block;width:38px;height:20px;flex-shrink:0}
.tog input{opacity:0;width:0;height:0}
.tog-sl{position:absolute;inset:0;background:#444;border-radius:20px;cursor:pointer;transition:.2s}
.tog-sl:before{content:'';position:absolute;width:14px;height:14px;left:3px;top:3px;background:#aaa;border-radius:50%;transition:.2s}
.tog input:checked+.tog-sl{background:#00bcd4}
.tog input:checked+.tog-sl:before{transform:translateX(18px);background:#fff}
.tog-lbl{color:#aaa;font-size:.85em;white-space:nowrap}
input[type=range]{flex:1;accent-color:#00bcd4;cursor:pointer}
input[type=range]:disabled{opacity:.3;cursor:default}
.bright-val{color:#888;font-size:.85em;min-width:28px;text-align:right}
@media(max-width:440px){.grid{grid-template-columns:1fr}}
</style>
</head>
<body>
<h1 id="h1">Ulanzi Clock</h1>
<div class="sub" id="sub">Connecting...</div>

<div class="grid">
  <div class="card"><div class="lbl">Role</div><div class="val" id="role">-</div></div>
  <div class="card"><div class="lbl">Uptime</div><div class="val" id="uptime">-</div></div>
  <div class="card"><div class="lbl">IP Address</div><div class="val green" id="ip">-</div></div>
  <div class="card"><div class="lbl">WiFi Channel</div><div class="val" id="ch">-</div></div>
  <div class="card"><div class="lbl">Battery</div><div class="val" id="bat">-</div></div>
  <div class="card"><div class="lbl">Light (LDR)</div><div class="val dim" id="ldr">-</div></div>
  <div class="card full"><div class="lbl">Time</div><div class="val cyan" id="clk">-</div></div>
  <div class="card"><div class="lbl" id="lbl-dmr">DMR</div><div class="val" id="dmr">-</div></div>
  <div class="card"><div class="lbl" id="lbl-poc">POCSAG</div><div class="val" id="poc">-</div></div>
  <div class="card full" id="card-msg">
    <div class="lbl">Last POCSAG</div>
    <div class="val amber" id="msg">-</div>
  </div>
  <div class="card full">
    <div class="lbl">Brightness</div>
    <div class="bright-row">
      <label class="tog">
        <input type="checkbox" id="tog-auto" onchange="onAutoToggle()">
        <span class="tog-sl"></span>
      </label>
      <span class="tog-lbl" id="tog-lbl">Auto</span>
      <input type="range" id="sld-bright" min="1" max="255" value="50" disabled
             oninput="document.getElementById('bright-val').textContent=this.value"
             onchange="onSliderChange()">
      <span class="bright-val" id="bright-val">50</span>
    </div>
  </div>
</div>

<script>
function fmtUp(s){
  var d=Math.floor(s/86400),h=Math.floor((s%86400)/3600),
      m=Math.floor((s%3600)/60),sc=s%60;
  return (d>0?d+'d ':'')+('0'+h).slice(-2)+':'+('0'+m).slice(-2)+':'+('0'+sc).slice(-2);
}
function poll(){
  fetch('/api/status').then(function(r){return r.json();}).then(function(d){
    document.getElementById('h1').textContent   = d.hostname;
    document.getElementById('sub').textContent  = d.hostname+' \u2022 '+d.ip;
    document.getElementById('role').textContent = d.role;
    document.getElementById('uptime').textContent = fmtUp(d.uptime);
    document.getElementById('ip').textContent   = d.ip;
    document.getElementById('ch').textContent   = 'ch '+d.channel;
    document.getElementById('clk').textContent  = d.time_synced ? d.time : 'not synced';
    var isSender = (d.role==='SENDER');
    document.getElementById('lbl-dmr').textContent  = isSender?'DMR Sent':'DMR Recv';
    document.getElementById('lbl-poc').textContent  = isSender?'POCSAG Sent':'POCSAG Recv';
    document.getElementById('dmr').textContent  = d.dmr_count;
    document.getElementById('poc').textContent  = d.pocsag_count;
    var msg = d.last_pocsag||'';
    document.getElementById('card-msg').style.display = (!isSender||msg)?'':'none';
    document.getElementById('msg').textContent = msg||'(none)';
    // Battery
    var batEl = document.getElementById('bat');
    var pct = d.battery_pct, mv = d.battery_mv;
    batEl.textContent = (mv/1000).toFixed(2)+' V \u00B7 '+pct+'%';
    batEl.className = 'val '+(pct>=60?'green':pct>=30?'amber':'red');
    // LDR
    document.getElementById('ldr').textContent = d.ldr_raw+' / 4095';
    // Brightness — only sync slider from server when auto is on (avoids overriding user drag)
    document.getElementById('tog-auto').checked = d.auto_brightness;
    document.getElementById('sld-bright').disabled = d.auto_brightness;
    document.getElementById('tog-lbl').textContent = d.auto_brightness ? 'Auto' : 'Manual';
    if (d.auto_brightness) {
      document.getElementById('sld-bright').value = d.brightness;
      document.getElementById('bright-val').textContent = d.brightness;
    }
  }).catch(function(){});
}
function postBright(isAuto, level) {
  fetch('/api/brightness', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: 'auto='+(isAuto?1:0)+'&level='+level
  }).catch(function(){});
}
function onAutoToggle() {
  var isAuto = document.getElementById('tog-auto').checked;
  var level  = document.getElementById('sld-bright').value;
  document.getElementById('sld-bright').disabled = isAuto;
  document.getElementById('tog-lbl').textContent = isAuto ? 'Auto' : 'Manual';
  postBright(isAuto, level);
}
function onSliderChange() {
  var level = document.getElementById('sld-bright').value;
  document.getElementById('bright-val').textContent = level;
  postBright(false, level);
}
poll();
setInterval(poll,2000);
</script>
</body>
</html>
)rawliteral";
