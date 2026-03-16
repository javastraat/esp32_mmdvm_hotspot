#pragma once
#include "styles.h"
#include "navigation.h"

// ── System page (/system): device, WiFi, hardware, software, reboot ───────
static const char PAGE_SYSTEM[] PROGMEM =
  "<!DOCTYPE html><html lang=\"en\">"
  "<head>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Ulanzi System</title>"
  "<style>" COMMON_CSS "</style>"
  THEME_INIT_SCRIPT
  "</head><body>"
  NAV_BAR
  NAV_LIVE_MODAL
  R"html(
<div class="container"><div class="grid">

  <div class="card">
    <h3>Device</h3>
    <div class="metric"><span class="metric-label">Hostname</span><span class="metric-value" id="hostname">-</span></div>
    <div class="metric"><span class="metric-label">IP Address</span><span class="metric-value" id="ip" style="color:#28a745">-</span></div>
    <div class="metric"><span class="metric-label">Uptime</span><span class="metric-value" id="uptime">-</span></div>
    <div class="metric"><span class="metric-label">Free Heap</span><span class="metric-value" id="heap">-</span></div>
  </div>

  <div class="card">
    <h3>WiFi</h3>
    <div class="metric"><span class="metric-label">SSID</span><span class="metric-value" id="ssid">-</span></div>
    <div class="metric"><span class="metric-label">Channel</span><span class="metric-value" id="ch">-</span></div>
    <div class="metric"><span class="metric-label">RSSI</span><span class="metric-value" id="rssi">-</span></div>
    <div class="metric"><span class="metric-label">MAC Address</span><span class="metric-value" id="mac">-</span></div>
  </div>

  <div class="card">
    <h3>Hardware</h3>
    <div class="metric"><span class="metric-label">Platform</span><span class="metric-value" id="hw-chip">-</span></div>
    <div class="metric"><span class="metric-label">Chip Rev</span><span class="metric-value" id="hw-rev">-</span></div>
    <div class="metric"><span class="metric-label">CPU</span><span class="metric-value" id="hw-cpu">-</span></div>
    <div class="metric"><span class="metric-label">CPU Temp</span><span class="metric-value" id="hw-temp">-</span></div>
    <div class="metric"><span class="metric-label">Flash</span><span class="metric-value" id="hw-flash">-</span></div>
    <div class="metric"><span class="metric-label">LED Matrix</span><span class="metric-value">32&#xd7;8 WS2812B</span></div>
    <div class="metric"><span class="metric-label">LDR</span><span class="metric-value">GL5516 GPIO35</span></div>
    <div class="metric"><span class="metric-label">Battery</span><span class="metric-value">4400 mAh LiPo</span></div>
  </div>

  <div class="card">
    <h3>Software</h3>
    <div class="metric"><span class="metric-label">Build</span><span class="metric-value" id="sw-build">-</span></div>
    <div class="metric"><span class="metric-label">SDK</span><span class="metric-value" id="sw-sdk">-</span></div>
    <div class="metric"><span class="metric-label">Reset Reason</span><span class="metric-value" id="sw-reset">-</span></div>
    <div class="metric"><span class="metric-label">Sketch</span><span class="metric-value" id="sw-sketch">-</span></div>
    <div class="metric"><span class="metric-label">OTA Space</span><span class="metric-value" id="sw-ota">-</span></div>
    <div class="metric"><span class="metric-label">webTask Stack</span><span class="metric-value" id="sw-stack">-</span></div>
    <div class="metric"><span class="metric-label">Min Free Heap</span><span class="metric-value" id="sw-minheap">-</span></div>
  </div>

  <div class="card">
    <h3>System</h3>
    <div class="metric">
      <span class="metric-label">Reboot device</span>
      <button onclick="doReboot()" style="background:#dc3545;color:#fff;border:none;padding:6px 16px;border-radius:4px;cursor:pointer;font-size:.85em;font-weight:bold">Reboot</button>
    </div>
  </div>

</div></div>
)html"
  "<script>" COMMON_JS NAV_LIVE_JS "</script>"
  R"html(
<script>
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
    document.getElementById('hostname').textContent=d.hostname;
    document.getElementById('ip').textContent=d.ip;
    document.getElementById('uptime').textContent=fmtUp(d.uptime);
    document.getElementById('heap').textContent=d.free_heap?Math.round(d.free_heap/1024)+' KB':'-';
    document.getElementById('ssid').textContent=d.ssid||'-';
    document.getElementById('ch').textContent='ch '+d.channel;
    document.getElementById('rssi').innerHTML=d.rssi?rssiBar(d.rssi):'-';
    document.getElementById('mac').textContent=d.mac||'-';
  }).catch(function(){});
}
function fetchSysInfo(){
  fetch('/api/sysinfo').then(function(r){return r.json();}).then(function(d){
    document.getElementById('hw-chip').textContent=d.chip_model||'-';
    document.getElementById('hw-rev').textContent='rev '+d.chip_rev;
    document.getElementById('hw-cpu').textContent=d.cpu_cores+' cores @ '+d.cpu_mhz+' MHz';
    document.getElementById('hw-temp').textContent=d.cpu_temp.toFixed(1)+' \u00b0C';
    document.getElementById('hw-flash').textContent=d.flash_mb+' MB';
    document.getElementById('sw-build').textContent=d.build||'-';
    document.getElementById('sw-sdk').textContent=d.sdk_version||'-';
    document.getElementById('sw-reset').textContent=d.reset_reason||'-';
    document.getElementById('sw-sketch').textContent=d.sketch_kb+' KB used';
    document.getElementById('sw-ota').textContent=d.free_sketch_kb+' KB free';
    var sf=d.webtask_stack_free;
    var sc=sf<512?'#f44336':sf<1024?'#ff9800':'';
    var se=document.getElementById('sw-stack');
    se.textContent=(sf>=1024?(sf/1024).toFixed(1)+' KB':sf+' B')+' free';
    if(sc)se.style.color=sc;
    document.getElementById('sw-minheap').textContent=
      d.min_free_heap?Math.round(d.min_free_heap/1024)+' KB':'-';
  }).catch(function(){});
}
function doReboot(){
  if(!confirm('Reboot Ulanzi?'))return;
  fetch('/api/reboot',{method:'POST'}).catch(function(){});
  setTimeout(function(){location.reload();},5000);
}
poll();
setInterval(poll,5000);
fetchSysInfo();
</script>
</body></html>
)html";
