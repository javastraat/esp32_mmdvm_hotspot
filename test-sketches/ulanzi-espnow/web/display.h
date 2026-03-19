#pragma once
#include "styles.h"
#include "navigation.h"

// ── Display page (/display): brightness, indicators, rotation, screensaver, icons, colors ──
static const char PAGE_DISPLAY[] PROGMEM =
  "<!DOCTYPE html><html lang=\"en\">"
  "<head>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Ulanzi Display</title>"
  "<style>" COMMON_CSS
  ".bp{background:#444;color:#fff;border:none;padding:5px 0;border-radius:4px;"
  "cursor:pointer;font-size:.82em;font-weight:bold;flex:1;min-width:52px;transition:background .15s}"
  ".bp:hover{background:#555}"
  "</style>"
  THEME_INIT_SCRIPT
  "</head><body>"
  NAV_BAR
  NAV_LIVE_MODAL
  R"html(
<div class="container"><div class="grid">

  <!-- Brightness -->
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
      <input type="range" id="sld-bright" min="0" max="255" value="50" disabled
             oninput="document.getElementById('bright-num').textContent=fmtBright(this.value);updatePresetButtons(this.value)"
             onchange="onSliderChange()">
      <span class="bright-num" id="bright-num">50</span>
    </div>
    <div id="bright-presets" style="display:flex;gap:6px;margin-top:8px;flex-wrap:wrap">
      <button id="bp-0"   onclick="applyPreset(0)"   class="bp">Off</button>
      <button id="bp-10"  onclick="applyPreset(10)"  class="bp">Night</button>
      <button id="bp-50"  onclick="applyPreset(50)"  class="bp">Dim</button>
      <button id="bp-120" onclick="applyPreset(120)" class="bp">Medium</button>
      <button id="bp-255" onclick="applyPreset(255)" class="bp">Bright</button>
    </div>
  </div>

  <!-- Indicators -->
  <div class="card">
    <h3>Indicators</h3>
    <div class="metric" style="margin-bottom:10px">
      <div>
        <span class="metric-label">Show status dots</span>
        <div style="font-size:.75em;color:var(--text-muted);margin-top:2px">
          3 pixels on the right edge: WiFi · POCSAG · MQTT
        </div>
      </div>
      <label class="switch" style="margin-left:12px;flex-shrink:0">
        <input type="checkbox" id="tog-ind" onchange="setIndicators(this.checked)">
        <span class="slider"></span>
      </label>
    </div>
    <div style="font-size:.78em;color:var(--text-muted);line-height:1.6">
      <div><span style="color:#00b400">&#9679;</span> WiFi connected &nbsp;/&nbsp; <span style="color:#c80000">&#9679; blink</span> disconnected</div>
      <div><span style="color:#ff6400">&#9679;</span> POCSAG received — 10 s flash</div>
      <div><span style="color:#00c8c8">&#9679;</span> MQTT connected &nbsp;/&nbsp; <span style="color:#c80000">&#9679;</span> disconnected</div>
    </div>
  </div>

  <!-- Rotate Screens -->
  <div class="card">
    <h3>Rotate Screens</h3>
    <div style="padding:6px 0;border-bottom:1px solid var(--border-color)">
      <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:8px">
        <span class="metric-label">Rotate Screens</span>
        <label class="switch">
          <input type="checkbox" id="tog-rot" onchange="onRotateChange()">
          <span class="slider"></span>
        </label>
      </div>
      <div class="bright-bot">
        <span class="metric-label" style="white-space:nowrap;flex-shrink:0">Rotate Interval</span>
        <input type="range" id="sld-rot" min="1" max="60" value="5"
               oninput="document.getElementById('rot-num').textContent=this.value+'s'"
               onchange="onRotateChange()">
        <span class="bright-num" id="rot-num">5s</span>
      </div>
    </div>
    <div class="metric" style="border-bottom:none;padding-top:8px">
      <span class="metric-label" style="font-size:.8em;color:var(--text-muted)">clock &#8594; temp &#8594; humidity &#8594; battery &#8594; clock</span>
    </div>
  </div>

  <!-- Screensaver -->
  <div class="card">
    <h3>Screensaver</h3>
    <div style="padding:6px 0;border-bottom:1px solid var(--border-color)">
      <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:8px">
        <span class="metric-label">Enable</span>
        <label class="switch">
          <input type="checkbox" id="tog-ss" onchange="onSsChange()">
          <span class="slider"></span>
        </label>
      </div>
      <div class="bright-bot">
        <span class="metric-label" style="white-space:nowrap;flex-shrink:0">Timeout</span>
        <input type="number" id="ss-timeout" min="5" max="3600" value="60"
               style="width:64px;background:var(--bg-secondary);color:var(--text-color);
                      border:1px solid var(--border-color);border-radius:4px;
                      padding:3px 6px;font-size:.88em"
               onchange="onSsChange()">
        <span style="font-size:.82em;color:var(--text-muted)">sec</span>
      </div>
    </div>
    <div class="metric" style="border-bottom:1px solid var(--border-color);padding:8px 0">
      <span class="metric-label">GIF</span>
      <select id="ss-file" onchange="onSsChange()"
              style="flex:1;margin-left:8px;padding:4px 6px;background:var(--bg-secondary);
                     color:var(--text-color);border:1px solid var(--border-color);
                     border-radius:4px;font-size:.88em">
        <option value="">(none)</option>
      </select>
    </div>
    <div style="padding:8px 0;border-bottom:1px solid var(--border-color)">
      <div style="font-size:.82em;color:var(--text-muted);margin-bottom:6px">Brightness</div>
      <div style="display:flex;gap:6px;margin-bottom:6px">
        <button id="ss-bp-a"  onclick="setSsBright(-2)" class="bp">Auto</button>
        <button id="ss-bp-0"  onclick="setSsBright(0)"  class="bp">Off</button>
        <button id="ss-bp-10" onclick="setSsBright(10)" class="bp">Night</button>
      </div>
      <div style="display:flex;gap:6px">
        <button id="ss-bp-50"  onclick="setSsBright(50)"  class="bp">Dim</button>
        <button id="ss-bp-120" onclick="setSsBright(120)" class="bp">Medium</button>
        <button id="ss-bp-255" onclick="setSsBright(255)" class="bp">Bright</button>
      </div>
    </div>
    <div style="font-size:.75em;color:var(--text-muted);padding:6px 0 8px">
      GIF must be exactly 32&#xd7;8 px. Place in /screensaver/ (visible in Files page).
    </div>
    <div id="ss-no-files" style="display:none;font-size:.82em;padding:4px 0 8px">
      No GIFs found in /screensaver/ &mdash; <a href="/files" style="color:#00bcd4">go to Files</a> to upload.
    </div>
    <div style="display:flex;align-items:center;justify-content:flex-end">
      <button id="btn-ss-test" onclick="testSs()"
              style="background:#444;color:#fff;border:none;padding:6px 18px;
                     border-radius:4px;cursor:pointer;font-weight:bold;font-size:.88em">
        Test
      </button>
    </div>
  </div>

  <!-- Icons -->
  <div class="card">
    <h3>Icons</h3>
    <div style="padding:8px 0;border-bottom:1px solid var(--border-color)">
      <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:5px">
        <span class="metric-label">Temp</span>
        <img id="prev-temp" src="" alt="" style="height:24px;width:auto;image-rendering:pixelated;border-radius:2px;display:none">
      </div>
      <div style="display:flex;gap:6px">
        <select id="icon-temp" onchange="onIconChange('icon-temp','prev-temp')"
                style="flex:1;min-width:0;padding:4px 6px;background:var(--bg-secondary);
                       color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;font-size:.88em">
          <option value="">(none)</option>
        </select>
        <button onclick="showIcon('icon-temp')"
                style="background:#444;color:#fff;border:none;padding:4px 10px;
                       border-radius:4px;cursor:pointer;font-size:.8em;flex-shrink:0">Show</button>
      </div>
    </div>
    <div style="padding:8px 0;border-bottom:1px solid var(--border-color)">
      <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:5px">
        <span class="metric-label">Humidity</span>
        <img id="prev-hum" src="" alt="" style="height:24px;width:auto;image-rendering:pixelated;border-radius:2px;display:none">
      </div>
      <div style="display:flex;gap:6px">
        <select id="icon-hum" onchange="onIconChange('icon-hum','prev-hum')"
                style="flex:1;min-width:0;padding:4px 6px;background:var(--bg-secondary);
                       color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;font-size:.88em">
          <option value="">(none)</option>
        </select>
        <button onclick="showIcon('icon-hum')"
                style="background:#444;color:#fff;border:none;padding:4px 10px;
                       border-radius:4px;cursor:pointer;font-size:.8em;flex-shrink:0">Show</button>
      </div>
    </div>
    <div style="padding:8px 0;border-bottom:1px solid var(--border-color)">
      <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:5px">
        <span class="metric-label">Battery</span>
        <img id="prev-bat" src="" alt="" style="height:24px;width:auto;image-rendering:pixelated;border-radius:2px;display:none">
      </div>
      <div style="display:flex;gap:6px">
        <select id="icon-bat" onchange="onIconChange('icon-bat','prev-bat')"
                style="flex:1;min-width:0;padding:4px 6px;background:var(--bg-secondary);
                       color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;font-size:.88em">
          <option value="">(none)</option>
        </select>
        <button onclick="showIcon('icon-bat')"
                style="background:#444;color:#fff;border:none;padding:4px 10px;
                       border-radius:4px;cursor:pointer;font-size:.8em;flex-shrink:0">Show</button>
      </div>
    </div>
    <div style="padding:8px 0">
      <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:5px">
        <span class="metric-label">POCSAG</span>
        <img id="prev-poc" src="" alt="" style="height:24px;width:auto;image-rendering:pixelated;border-radius:2px;display:none">
      </div>
      <div style="display:flex;gap:6px">
        <select id="icon-poc" onchange="onIconChange('icon-poc','prev-poc')"
                style="flex:1;min-width:0;padding:4px 6px;background:var(--bg-secondary);
                       color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;font-size:.88em">
          <option value="">(none)</option>
        </select>
        <button onclick="showIcon('icon-poc')"
                style="background:#444;color:#fff;border:none;padding:4px 10px;
                       border-radius:4px;cursor:pointer;font-size:.8em;flex-shrink:0">Show</button>
      </div>
    </div>
    <div id="icon-no-files" style="display:none;font-size:.82em;padding:4px 0 8px">
      No icons found in /icons/ &mdash; <a href="/files" style="color:#00bcd4">go to Files</a> to upload.
    </div>
    <div style="display:flex;align-items:center;justify-content:flex-end;gap:10px;margin-top:10px">
      <span id="icon-status" style="font-size:.82em;color:var(--text-muted)"></span>
      <button onclick="saveIcons()"
              style="background:#00bcd4;color:#000;border:none;padding:6px 18px;
                     border-radius:4px;cursor:pointer;font-weight:bold;font-size:.88em">
        Save Icons
      </button>
    </div>
  </div>

  <!-- Text Colors -->
  <div class="card">
    <h3>Text Colors</h3>
    <div class="metric">
      <span class="metric-label" style="flex-shrink:0;width:72px">Clock</span>
      <input type="color" id="col-clock" value="#FFFFFF" onchange="saveColors()">
    </div>
    <div class="metric" style="border-bottom:none">
      <span class="metric-label" style="flex-shrink:0;width:72px">POCSAG</span>
      <input type="color" id="col-poc" value="#FFA000" onchange="saveColors()">
    </div>
  </div>

  <!-- Thresholds & Colors -->
  <div class="card">
    <h3>Thresholds &amp; Colors</h3>
    <div style="padding:6px 0;border-bottom:1px solid var(--border-color)">
      <div style="font-size:.82em;font-weight:bold;margin-bottom:6px">Temperature (&deg;C)</div>
      <div style="display:flex;align-items:center;gap:6px;margin-bottom:6px;flex-wrap:wrap">
        <span class="metric-label" style="flex-shrink:0;width:72px">Thresholds</span>
        <input type="number" id="t-thr-lo" min="-40" max="60" step="0.5"
               style="width:52px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:3px 6px;font-size:.88em"
               onchange="saveThresholds()">
        <span style="color:var(--text-muted)">&#8211;</span>
        <input type="number" id="t-thr-hi" min="-40" max="60" step="0.5"
               style="width:52px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:3px 6px;font-size:.88em"
               onchange="saveThresholds()">
      </div>
      <div style="margin-top:4px">
        <div style="font-size:.75em;color:var(--text-muted);margin-bottom:3px">Colors</div>
        <div style="display:flex;align-items:center;gap:8px">
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">Low<input type="color" id="t-col-lo" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">Mid<input type="color" id="t-col-mid" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">High<input type="color" id="t-col-hi" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
        </div>
      </div>
    </div>
    <div style="padding:6px 0;border-bottom:1px solid var(--border-color)">
      <div style="font-size:.82em;font-weight:bold;margin-bottom:6px">Humidity (%)</div>
      <div style="display:flex;align-items:center;gap:6px;margin-bottom:6px;flex-wrap:wrap">
        <span class="metric-label" style="flex-shrink:0;width:72px">Thresholds</span>
        <input type="number" id="h-thr-lo" min="0" max="100" step="1"
               style="width:52px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:3px 6px;font-size:.88em"
               onchange="saveThresholds()">
        <span style="color:var(--text-muted)">&#8211;</span>
        <input type="number" id="h-thr-hi" min="0" max="100" step="1"
               style="width:52px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:3px 6px;font-size:.88em"
               onchange="saveThresholds()">
      </div>
      <div style="margin-top:4px">
        <div style="font-size:.75em;color:var(--text-muted);margin-bottom:3px">Colors</div>
        <div style="display:flex;align-items:center;gap:8px">
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">Low<input type="color" id="h-col-lo" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">Mid<input type="color" id="h-col-mid" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">High<input type="color" id="h-col-hi" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
        </div>
      </div>
    </div>
    <div style="padding:6px 0">
      <div style="font-size:.82em;font-weight:bold;margin-bottom:6px">Battery (%)</div>
      <div style="display:flex;align-items:center;gap:6px;margin-bottom:6px;flex-wrap:wrap">
        <span class="metric-label" style="flex-shrink:0;width:72px">Thresholds</span>
        <input type="number" id="b-thr-lo" min="0" max="100" step="1"
               style="width:52px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:3px 6px;font-size:.88em"
               onchange="saveThresholds()">
        <span style="color:var(--text-muted)">&#8211;</span>
        <input type="number" id="b-thr-hi" min="0" max="100" step="1"
               style="width:52px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:3px 6px;font-size:.88em"
               onchange="saveThresholds()">
      </div>
      <div style="margin-top:4px">
        <div style="font-size:.75em;color:var(--text-muted);margin-bottom:3px">Colors</div>
        <div style="display:flex;align-items:center;gap:8px">
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">Low<input type="color" id="b-col-lo" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">Mid<input type="color" id="b-col-mid" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">High<input type="color" id="b-col-hi" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
        </div>
      </div>
    </div>
  </div>

</div></div>
)html"
  "<script>" COMMON_JS NAV_LIVE_JS "</script>"
  R"html(
<script>
var PRESETS=[0,10,50,120,255];
function fmtBright(v){return parseInt(v)===0?'Off':v;}
function updatePresetButtons(val){
  val=parseInt(val);
  PRESETS.forEach(function(v){
    var b=document.getElementById('bp-'+v);
    if(!b)return;
    var active=(val===v);
    b.style.background=active?'#00bcd4':'#444';
    b.style.color=active?'#000':'#fff';
  });
}
function applyPreset(val){
  var tog=document.getElementById('tog-auto');
  if(tog.checked){
    tog.checked=false;
    document.getElementById('sld-bright').disabled=false;
    document.getElementById('tog-lbl').textContent='Manual';
    stopAutoPoller();
  }
  var sld=document.getElementById('sld-bright');
  sld.value=val;
  document.getElementById('bright-num').textContent=fmtBright(val);
  updatePresetButtons(val);
  postBright(false,val);
}
function postBright(isAuto,level){
  fetch('/api/brightness',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'auto='+(isAuto?1:0)+'&level='+level
  }).catch(function(){});
}
var _autoPoller=null;
function startAutoPoller(){
  if(_autoPoller)return;
  _autoPoller=setInterval(function(){
    fetch('/api/status').then(function(r){return r.json();}).then(function(d){
      if(!document.getElementById('tog-auto').checked){stopAutoPoller();return;}
      document.getElementById('sld-bright').value=d.brightness;
      document.getElementById('bright-num').textContent=fmtBright(d.brightness);
      updatePresetButtons(d.brightness);
    }).catch(function(){});
  },800);
}
function stopAutoPoller(){
  if(_autoPoller){clearInterval(_autoPoller);_autoPoller=null;}
}
function onAutoToggle(){
  var isAuto=document.getElementById('tog-auto').checked;
  document.getElementById('sld-bright').disabled=isAuto;
  document.getElementById('tog-lbl').textContent=isAuto?'Auto':'Manual';
  postBright(isAuto,document.getElementById('sld-bright').value);
  if(isAuto){startAutoPoller();}else{stopAutoPoller();}
}
function onSliderChange(){
  var v=document.getElementById('sld-bright').value;
  document.getElementById('bright-num').textContent=fmtBright(v);
  updatePresetButtons(v);
  postBright(false,v);
}
function onRotateChange(){
  fetch('/api/rotate',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'enabled='+(document.getElementById('tog-rot').checked?1:0)
        +'&interval='+document.getElementById('sld-rot').value
  }).catch(function(){});
}
function onSsChange(){
  fetch('/api/screensaver',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'enabled='+(document.getElementById('tog-ss').checked?1:0)
        +'&timeout='+document.getElementById('ss-timeout').value
        +'&file='+encodeURIComponent(document.getElementById('ss-file').value)
  }).catch(function(){});
}
var _ssBright=-1;
var SS_BP_IDS={'-2':'ss-bp-a','0':'ss-bp-0','10':'ss-bp-10','50':'ss-bp-50','120':'ss-bp-120','255':'ss-bp-255'};
function updateSsBrightButtons(val){
  _ssBright=val;
  Object.keys(SS_BP_IDS).forEach(function(k){
    var b=document.getElementById(SS_BP_IDS[k]);
    if(!b)return;
    var active=(parseInt(k)===val);
    b.style.background=active?'#00bcd4':'#444';
    b.style.color=active?'#000':'#fff';
  });
}
function setSsBright(val){
  updateSsBrightButtons(val);
  fetch('/api/screensaver',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'brightness='+val
  }).catch(function(){});
}
function testSs(){
  var btn=document.getElementById('btn-ss-test');
  var isTesting=(btn.textContent.trim()==='Test');
  fetch('/api/screensaver/test',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'action='+(isTesting?'test':'stop')
  }).then(function(){
    btn.textContent=isTesting?'Stop':'Test';
    btn.style.background=isTesting?'#dc3545':'#444';
  }).catch(function(){});
}
function updateIconPreview(selId,imgId){
  var path=document.getElementById(selId).value;
  var img=document.getElementById(imgId);
  if(path){img.src='/api/fs/download?path='+encodeURIComponent(path);img.style.display='inline-block';}
  else{img.src='';img.style.display='none';}
}
function onIconChange(selId,imgId){updateIconPreview(selId,imgId);}
function showIcon(selId){
  var path=document.getElementById(selId).value;
  if(!path)return;
  fetch('/api/icons/preview',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'path='+encodeURIComponent(path)
  }).catch(function(){});
}
function populateIconSelect(selId,imgId,files,current){
  var sel=document.getElementById(selId);
  sel.innerHTML='<option value="">(none)</option>';
  var found=false;
  files.forEach(function(f){
    var opt=document.createElement('option');
    opt.value=f.path;opt.textContent=f.name;
    if(f.path===current){opt.selected=true;found=true;}
    sel.appendChild(opt);
  });
  if(current&&!found){
    var opt=document.createElement('option');
    opt.value=current;
    var sl=current.lastIndexOf('/');
    opt.textContent=(sl>=0?current.substring(sl+1):current)+' (saved)';
    opt.selected=true;sel.appendChild(opt);
  }
  updateIconPreview(selId,imgId);
}
function saveIcons(){
  var body='temp_icon='+encodeURIComponent(document.getElementById('icon-temp').value)
          +'&hum_icon='+encodeURIComponent(document.getElementById('icon-hum').value)
          +'&bat_icon='+encodeURIComponent(document.getElementById('icon-bat').value)
          +'&poc_icon='+encodeURIComponent(document.getElementById('icon-poc').value);
  fetch('/api/icons',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})
  .then(function(r){return r.json();}).then(function(d){
    var s=document.getElementById('icon-status');
    s.textContent=d.ok?'Saved':'Error';s.style.color=d.ok?'#28a745':'#dc3545';
    setTimeout(function(){s.textContent='';},2500);
  }).catch(function(){});
}
function saveColors(){
  fetch('/api/colors',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'clock='+encodeURIComponent(document.getElementById('col-clock').value)
        +'&poc='+encodeURIComponent(document.getElementById('col-poc').value)
  }).catch(function(){});
}
function saveThresholds(){
  fetch('/api/colors',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'tmp_thr_lo='+document.getElementById('t-thr-lo').value
        +'&tmp_thr_hi='+document.getElementById('t-thr-hi').value
        +'&t_lo='+encodeURIComponent(document.getElementById('t-col-lo').value)
        +'&t_mid='+encodeURIComponent(document.getElementById('t-col-mid').value)
        +'&t_hi='+encodeURIComponent(document.getElementById('t-col-hi').value)
        +'&hum_thr_lo='+document.getElementById('h-thr-lo').value
        +'&hum_thr_hi='+document.getElementById('h-thr-hi').value
        +'&h_lo='+encodeURIComponent(document.getElementById('h-col-lo').value)
        +'&h_mid='+encodeURIComponent(document.getElementById('h-col-mid').value)
        +'&h_hi='+encodeURIComponent(document.getElementById('h-col-hi').value)
        +'&bat_thr_lo='+document.getElementById('b-thr-lo').value
        +'&bat_thr_hi='+document.getElementById('b-thr-hi').value
        +'&b_lo='+encodeURIComponent(document.getElementById('b-col-lo').value)
        +'&b_mid='+encodeURIComponent(document.getElementById('b-col-mid').value)
        +'&b_hi='+encodeURIComponent(document.getElementById('b-col-hi').value)
  }).catch(function(){});
}
function setIndicators(val){
  fetch('/api/indicators',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'enabled='+(val?1:0)
  }).catch(function(){});
}
(function init(){
  fetch('/api/status').then(function(r){return r.json();}).then(function(d){
    document.getElementById('h1').textContent=d.hostname;
    document.getElementById('sub').textContent=d.ip;
    document.getElementById('tog-auto').checked=d.auto_brightness;
    document.getElementById('sld-bright').disabled=d.auto_brightness;
    document.getElementById('tog-lbl').textContent=d.auto_brightness?'Auto':'Manual';
    document.getElementById('sld-bright').value=d.brightness;
    document.getElementById('bright-num').textContent=fmtBright(d.brightness);
    updatePresetButtons(d.brightness);
    if(d.auto_brightness)startAutoPoller();
    var ri=d.rotate_interval||5;
    document.getElementById('tog-rot').checked=d.rotate_enabled;
    document.getElementById('sld-rot').value=ri;
    document.getElementById('rot-num').textContent=ri+'s';
  }).catch(function(){});
  fetch('/api/indicators').then(function(r){return r.json();}).then(function(d){
    document.getElementById('tog-ind').checked=d.enabled;
  }).catch(function(){});
  fetch('/api/icons').then(function(r){return r.json();}).then(function(d){
    fetch('/api/fs/ls?path=/icons').then(function(r){return r.json();}).then(function(ls){
      var files=(ls.entries||[]).filter(function(e){return !e.isDir&&/\.(gif|jpg|jpeg)$/i.test(e.name);});
      if(files.length===0){document.getElementById('icon-no-files').style.display='block';}
      populateIconSelect('icon-temp','prev-temp',files,d.temp||'');
      populateIconSelect('icon-hum', 'prev-hum', files,d.hum||'');
      populateIconSelect('icon-bat', 'prev-bat', files,d.bat||'');
      populateIconSelect('icon-poc', 'prev-poc', files,d.poc||'');
    }).catch(function(){});
  }).catch(function(){});
  fetch('/api/colors').then(function(r){return r.json();}).then(function(d){
    if(d.clock)document.getElementById('col-clock').value=d.clock;
    if(d.poc)document.getElementById('col-poc').value=d.poc;
    if(d.tmp_thr_lo!==undefined)document.getElementById('t-thr-lo').value=d.tmp_thr_lo;
    if(d.tmp_thr_hi!==undefined)document.getElementById('t-thr-hi').value=d.tmp_thr_hi;
    if(d.t_lo)document.getElementById('t-col-lo').value=d.t_lo;
    if(d.t_mid)document.getElementById('t-col-mid').value=d.t_mid;
    if(d.t_hi)document.getElementById('t-col-hi').value=d.t_hi;
    if(d.hum_thr_lo!==undefined)document.getElementById('h-thr-lo').value=d.hum_thr_lo;
    if(d.hum_thr_hi!==undefined)document.getElementById('h-thr-hi').value=d.hum_thr_hi;
    if(d.h_lo)document.getElementById('h-col-lo').value=d.h_lo;
    if(d.h_mid)document.getElementById('h-col-mid').value=d.h_mid;
    if(d.h_hi)document.getElementById('h-col-hi').value=d.h_hi;
    if(d.bat_thr_lo!==undefined)document.getElementById('b-thr-lo').value=d.bat_thr_lo;
    if(d.bat_thr_hi!==undefined)document.getElementById('b-thr-hi').value=d.bat_thr_hi;
    if(d.b_lo)document.getElementById('b-col-lo').value=d.b_lo;
    if(d.b_mid)document.getElementById('b-col-mid').value=d.b_mid;
    if(d.b_hi)document.getElementById('b-col-hi').value=d.b_hi;
  }).catch(function(){});
  fetch('/api/screensaver').then(function(r){return r.json();}).then(function(d){
    document.getElementById('tog-ss').checked=d.enabled;
    document.getElementById('ss-timeout').value=d.timeout||60;
    updateSsBrightButtons(d.brightness!=null?d.brightness:-1);
    fetch('/api/fs/ls?path=/screensaver').then(function(r){return r.json();}).then(function(ls){
      var files=(ls.entries||[]).filter(function(e){return !e.isDir&&/\.gif$/i.test(e.name);});
      if(files.length===0){document.getElementById('ss-no-files').style.display='block';}
      var sel=document.getElementById('ss-file');
      files.forEach(function(e){
        var opt=document.createElement('option');
        opt.value=e.path;opt.textContent=e.name;
        if(e.path===d.file)opt.selected=true;
        sel.appendChild(opt);
      });
      if(d.file&&!sel.value)sel.value='';
    }).catch(function(){});
  }).catch(function(){});
})();
</script>
</body></html>
)html";
