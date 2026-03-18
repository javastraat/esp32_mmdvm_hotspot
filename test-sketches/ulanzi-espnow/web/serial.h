#pragma once
#include "styles.h"
#include "navigation.h"

// ── Serial page (/serial): live mirror of Serial output via ring-buffer polling ──
static const char PAGE_SERIAL[] PROGMEM =
  "<!DOCTYPE html><html lang=\"en\">"
  "<head>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Ulanzi Serial</title>"
  "<style>" COMMON_CSS "</style>"
  THEME_INIT_SCRIPT
  "</head><body>"
  NAV_BAR
  NAV_LIVE_MODAL
  R"html(
<div class="container">

  <div class="card">
    <h3>Serial Monitor</h3>
    <div style="display:flex;gap:8px;margin-bottom:10px;align-items:center;flex-wrap:wrap">
      <button id="pause-btn" class="btn btn-secondary" onclick="togglePause()">&#9646;&#9646; Pause</button>
      <button class="btn btn-warning" onclick="doClear()">&#10005; Clear</button>
      <button class="btn btn-secondary" onclick="doScrollBottom()">&#8595; Bottom</button>
      <span id="status" class="metric-label" style="margin-left:4px;font-family:monospace"></span>
    </div>
    <pre id="log-out"
      style="background:#0d1117;color:#3fb950;font-family:monospace;font-size:.78em;
             padding:12px;border-radius:6px;height:500px;overflow-y:auto;
             white-space:pre-wrap;word-break:break-all;margin:0;
             border:1px solid var(--border-color);line-height:1.45">Waiting for output...</pre>
  </div>

</div>
)html"
  "<script>" COMMON_JS NAV_LIVE_JS "</script>"
  R"html(
<script>
var cursor=0;
var paused=false;
var firstData=true;

function togglePause(){
  paused=!paused;
  var btn=document.getElementById('pause-btn');
  btn.textContent=paused?'\u25BA Resume':'\u23F8 Pause';
  btn.className=paused?'btn btn-success':'btn btn-secondary';
}

function doClear(){
  showConfirm('Clear the serial log buffer on device?',function(){
    fetch('/api/serial/clear',{method:'POST'}).catch(function(){});
    document.getElementById('log-out').textContent='';
    cursor=0;
    firstData=true;
    document.getElementById('status').textContent='';
  });
}

function doScrollBottom(){
  var el=document.getElementById('log-out');
  el.scrollTop=el.scrollHeight;
}

function poll(){
  if(paused){setTimeout(poll,1000);return;}
  fetch('/api/serial/log?cursor='+cursor)
  .then(function(r){return r.json();})
  .then(function(d){
    if(d.data&&d.data.length>0){
      var el=document.getElementById('log-out');
      var atBottom=el.scrollHeight-el.scrollTop<=el.clientHeight+20;
      if(firstData){el.textContent='';firstData=false;}
      el.textContent+=d.data;
      // Trim if content grows too large — keep last 50 KB
      if(el.textContent.length>60000)
        el.textContent=el.textContent.slice(-50000);
      if(atBottom)el.scrollTop=el.scrollHeight;
    }
    cursor=d.cursor;
    document.getElementById('status').textContent=cursor+' B';
  })
  .catch(function(){})
  .finally(function(){setTimeout(poll,1000);});
}
poll();
</script>
</body></html>
)html";
