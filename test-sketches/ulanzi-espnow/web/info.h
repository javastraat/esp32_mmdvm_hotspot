#pragma once
#include "styles.h"
#include "navigation.h"

// ── Info page (/info): device, WiFi, hardware, software info ─────────────
static const char PAGE_INFO[] PROGMEM =
  "<!DOCTYPE html><html lang=\"en\">"
  "<head>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Ulanzi Info</title>"
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
    <div class="metric"><span class="metric-label">mDNS</span><span class="metric-value" id="mdns-status">-</span></div>
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
    <div class="metric"><span class="metric-label">Arduino Core</span><span class="metric-value" id="sw-arduino">-</span></div>
    <div class="metric"><span class="metric-label">Reset Reason</span><span class="metric-value" id="sw-reset">-</span></div>
    <div class="metric"><span class="metric-label">Sketch</span><span class="metric-value" id="sw-sketch">-</span></div>
    <div class="metric"><span class="metric-label">OTA Space</span><span class="metric-value" id="sw-ota">-</span></div>
    <div class="metric"><span class="metric-label">webTask Stack</span><span class="metric-value" id="sw-stack">-</span></div>
    <div class="metric"><span class="metric-label">Min Free Heap</span><span class="metric-value" id="sw-minheap">-</span></div>
  </div>

  <div class="card">
    <h3>Memory</h3>
    <div class="metric"><span class="metric-label">Heap Size</span><span class="metric-value" id="mem-heap">-</span></div>
    <div class="metric"><span class="metric-label">Free Heap</span><span class="metric-value" id="mem-free">-</span></div>
    <div class="metric"><span class="metric-label">Max Alloc</span><span class="metric-value" id="mem-maxalloc">-</span></div>
    <div class="metric"><span class="metric-label">Min Free</span><span class="metric-value" id="mem-minfree">-</span></div>
    <div id="mem-psram-row" style="display:none">
      <div class="metric"><span class="metric-label">PSRAM Size</span><span class="metric-value" id="mem-psram-size">-</span></div>
      <div class="metric"><span class="metric-label">Free PSRAM</span><span class="metric-value" id="mem-psram-free">-</span></div>
    </div>
  </div>

  <div class="card">
    <h3>Storage</h3>
    <div class="metric"><span class="metric-label">Flash Speed</span><span class="metric-value" id="stor-speed">-</span></div>
    <div class="metric"><span class="metric-label">Flash Mode</span><span class="metric-value" id="stor-mode">-</span></div>
    <div class="metric"><span class="metric-label">Partition</span><span class="metric-value" id="stor-part">-</span></div>
    <div class="metric"><span class="metric-label">Sketch MD5</span><span class="metric-value" id="stor-md5" style="font-size:0.78em;font-family:monospace">-</span></div>
  </div>

  <div class="card">
    <h3>LittleFS</h3>
    <div class="metric"><span class="metric-label">Status</span><span class="metric-value" id="lfs-status">-</span></div>
    <div class="metric"><span class="metric-label">Total</span><span class="metric-value" id="lfs-total">-</span></div>
    <div class="metric"><span class="metric-label">Used</span><span class="metric-value" id="lfs-used">-</span></div>
    <div class="metric"><span class="metric-label">Free</span><span class="metric-value" id="lfs-free">-</span></div>
  </div>

  <div class="card">
    <h3>Task Stacks</h3>
    <div class="metric"><span class="metric-label">webTask</span><span class="metric-value" id="task-web">-</span></div>
    <div class="metric"><span class="metric-label">mqttTask</span><span class="metric-value" id="task-mqtt">-</span></div>
  </div>

  <div class="card">
    <h3>NVS Namespace Listing</h3>
    <p style="font-size:0.85em;color:var(--text-muted);margin:0 0 10px">Click a namespace to view its stored keys.</p>
    <div id="nvs-ns-list"><span style="color:var(--text-muted);font-size:0.85em">Loading&hellip;</span></div>
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
function fmtKB(b){return Math.round(b/1024)+' KB';}
function stackFmt(b){return b>=1024?(b/1024).toFixed(1)+' KB free':b+' B free';}
function stackColor(b){return b<512?'#f44336':b<1024?'#ff9800':'';}
function fetchSysInfo(){
  fetch('/api/sysinfo').then(function(r){return r.json();}).then(function(d){
    var ms=document.getElementById('mdns-status');
    if(d.mdns_started){
      ms.textContent=d.mdns_name+'.local';
      ms.style.color='#28a745';
    } else {
      ms.textContent='FAILED — IP only';
      ms.style.color='#dc3545';
    }
    document.getElementById('hw-chip').textContent=d.chip_model||'-';
    document.getElementById('hw-rev').textContent='rev '+d.chip_rev;
    document.getElementById('hw-cpu').textContent=d.cpu_cores+' cores @ '+d.cpu_mhz+' MHz';
    document.getElementById('hw-temp').textContent=d.cpu_temp.toFixed(1)+' \u00b0C';
    document.getElementById('hw-flash').textContent=d.flash_mb+' MB';
    document.getElementById('sw-build').textContent=d.build||'-';
    document.getElementById('sw-sdk').textContent=d.sdk_version||'-';
    document.getElementById('sw-arduino').textContent=d.arduino_version||'-';
    document.getElementById('sw-reset').textContent=d.reset_reason||'-';
    document.getElementById('sw-sketch').textContent=d.sketch_kb+' KB used';
    document.getElementById('sw-ota').textContent=d.free_sketch_kb+' KB free';
    var sf=d.webtask_stack_free;
    var sc=stackColor(sf);
    var se=document.getElementById('sw-stack');
    se.textContent=stackFmt(sf);
    if(sc)se.style.color=sc;
    document.getElementById('sw-minheap').textContent=
      d.min_free_heap?fmtKB(d.min_free_heap):'-';
    // Memory card
    document.getElementById('mem-heap').textContent=fmtKB(d.heap_size);
    var fp=d.heap_size>0?Math.round(d.free_heap*100/d.heap_size):0;
    document.getElementById('mem-free').textContent=fmtKB(d.free_heap)+' ('+fp+'%)';
    document.getElementById('mem-maxalloc').textContent=fmtKB(d.max_alloc_heap);
    document.getElementById('mem-minfree').textContent=fmtKB(d.min_free_heap);
    if(d.psram_size>0){
      document.getElementById('mem-psram-row').style.display='';
      document.getElementById('mem-psram-size').textContent=Math.round(d.psram_size/1048576)+' MB';
      document.getElementById('mem-psram-free').textContent=fmtKB(d.free_psram);
    }
    // Storage card
    document.getElementById('stor-speed').textContent=d.flash_speed_mhz+' MHz';
    document.getElementById('stor-mode').textContent=d.flash_mode||'-';
    document.getElementById('stor-part').textContent=d.running_partition||'-';
    document.getElementById('stor-md5').textContent=d.sketch_md5||'-';
  }).catch(function(){});
}
function fetchFs(){
  fetch('/api/fs').then(function(r){return r.json();}).then(function(d){
    var st=document.getElementById('lfs-status');
    st.textContent='Available'; st.style.color='#28a745';
    var pct=d.total>0?Math.round(d.used*100/d.total):0;
    document.getElementById('lfs-total').textContent=fmtKB(d.total);
    document.getElementById('lfs-used').textContent=fmtKB(d.used)+' ('+pct+'%)';
    document.getElementById('lfs-free').textContent=fmtKB(d.available);
  }).catch(function(){
    var st=document.getElementById('lfs-status');
    st.textContent='Unavailable'; st.style.color='#dc3545';
  });
}
function fetchTasks(){
  fetch('/api/tasks').then(function(r){return r.json();}).then(function(d){
    [['webTask','task-web'],['mqttTask','task-mqtt']].forEach(function(p){
      var b=d[p[0]]||0;
      var el=document.getElementById(p[1]);
      el.textContent=stackFmt(b);
      var c=stackColor(b); if(c)el.style.color=c;
    });
  }).catch(function(){});
}
poll();
setInterval(poll,5000);
fetchSysInfo();
fetchFs();
fetchTasks();
fetchNvsNamespaces();
function fetchNvsNamespaces(){
  var el=document.getElementById('nvs-ns-list');
  fetch('/api/nvs/namespaces').then(function(r){return r.json();}).then(function(list){
    if(!list.length){el.innerHTML='<span style="color:var(--text-muted);font-size:0.85em">No namespaces found.</span>';return;}
    var html='<div style="display:flex;flex-direction:column;gap:6px">';
    list.forEach(function(ns){
      html+='<button onclick="showNvsKeys(\''+ns+'\')" style="text-align:left;padding:6px 12px;border-radius:4px;border:1px solid var(--border-color);background:var(--card-bg);color:var(--text-color);cursor:pointer;font-family:monospace;font-size:0.88em">'+ns+'</button>';
    });
    html+='</div>';
    el.innerHTML=html;
  }).catch(function(){el.innerHTML='<span style="color:#dc3545;font-size:0.85em">Failed to load namespaces.</span>';});
}
function showNvsKeys(ns){
  var container=document.querySelector('.container');
  container.innerHTML='<div class="grid"><div class="card" style="grid-column:1/-1">'
    +'<button onclick="location.reload()" style="margin-bottom:14px;padding:6px 16px;border-radius:4px;border:1px solid var(--border-color);background:var(--card-bg);color:var(--text-color);cursor:pointer;font-size:0.88em">&larr; Back</button>'
    +'<h3 style="margin-top:0">NVS &mdash; <span style="font-family:monospace">'+ns+'</span></h3>'
    +'<div id="nvs-keys-body"><span style="color:var(--text-muted);font-size:0.85em">Loading&hellip;</span></div>'
    +'</div></div>';
  window.scrollTo(0,0);
  fetch('/api/nvs/keys?ns='+encodeURIComponent(ns)).then(function(r){return r.json();}).then(function(keys){
    var el=document.getElementById('nvs-keys-body');
    if(!keys.length){el.innerHTML='<span style="color:var(--text-muted);font-size:0.85em">No keys found in this namespace.</span>';return;}
    var html='<table style="width:100%;border-collapse:collapse;font-size:0.85em">'
      +'<thead><tr style="border-bottom:2px solid var(--border-color)">'
      +'<th style="text-align:left;padding:6px 10px;color:var(--text-muted)">Key</th>'
      +'<th style="text-align:left;padding:6px 10px;color:var(--text-muted)">Type</th>'
      +'<th style="text-align:left;padding:6px 10px;color:var(--text-muted)">Value</th>'
      +'</tr></thead><tbody>';
    keys.forEach(function(kv){
      var val=kv.v==='***'?'<span style="letter-spacing:3px;color:var(--text-muted)">&bull;&bull;&bull;</span>':escHtml(kv.v);
      html+='<tr style="border-bottom:1px solid var(--border-color)">'
        +'<td style="padding:5px 10px;font-family:monospace">'+escHtml(kv.k)+'</td>'
        +'<td style="padding:5px 10px;color:var(--text-muted)">'+escHtml(kv.t)+'</td>'
        +'<td style="padding:5px 10px;font-family:monospace;word-break:break-all">'+val+'</td>'
        +'</tr>';
    });
    html+='</tbody></table>';
    el.innerHTML=html;
  }).catch(function(){
    var el=document.getElementById('nvs-keys-body');
    if(el) el.innerHTML='<span style="color:#dc3545;font-size:0.85em">Error loading keys.</span>';
  });
}
function escHtml(s){
  return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}
</script>
</body></html>
)html";
