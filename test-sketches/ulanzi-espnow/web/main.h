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
.full{grid-column:1/-1}
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
  <div class="card full"><div class="lbl">Time</div><div class="val cyan" id="clk">-</div></div>
  <div class="card"><div class="lbl" id="lbl-dmr">DMR</div><div class="val" id="dmr">-</div></div>
  <div class="card"><div class="lbl" id="lbl-poc">POCSAG</div><div class="val" id="poc">-</div></div>
  <div class="card full" id="card-msg">
    <div class="lbl">Last POCSAG</div>
    <div class="val amber" id="msg">-</div>
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
  }).catch(function(){});
}
poll();
setInterval(poll,2000);
</script>
</body>
</html>
)rawliteral";
