#pragma once
#include "styles.h"
#include "navigation.h"

// ── Settings page (/settings): brightness, buzzer, display rotation ───────
static const char PAGE_SETTINGS[] PROGMEM =
  "<!DOCTYPE html><html lang=\"en\">"
  "<head>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Ulanzi Settings</title>"
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

  <!-- Display Rotation -->
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
    <div class="metric">
      <span class="metric-label" style="flex-shrink:0;width:68px">Temp</span>
      <select id="icon-temp" onchange="onIconChange('icon-temp','prev-temp')"
              style="flex:1;padding:4px 6px;background:var(--bg-secondary);
                     color:var(--text-color);border:1px solid var(--border-color);
                     border-radius:4px;font-size:.88em">
        <option value="">(none)</option>
      </select>
      <img id="prev-temp" src="" alt=""
           style="height:28px;width:auto;image-rendering:pixelated;margin-left:6px;
                  border-radius:2px;display:none">
      <button onclick="showIcon('icon-temp')"
              style="background:#444;color:#fff;border:none;padding:4px 10px;
                     border-radius:4px;cursor:pointer;font-size:.8em;flex-shrink:0;margin-left:6px">
        Show
      </button>
    </div>
    <div class="metric">
      <span class="metric-label" style="flex-shrink:0;width:68px">Humidity</span>
      <select id="icon-hum" onchange="onIconChange('icon-hum','prev-hum')"
              style="flex:1;padding:4px 6px;background:var(--bg-secondary);
                     color:var(--text-color);border:1px solid var(--border-color);
                     border-radius:4px;font-size:.88em">
        <option value="">(none)</option>
      </select>
      <img id="prev-hum" src="" alt=""
           style="height:28px;width:auto;image-rendering:pixelated;margin-left:6px;
                  border-radius:2px;display:none">
      <button onclick="showIcon('icon-hum')"
              style="background:#444;color:#fff;border:none;padding:4px 10px;
                     border-radius:4px;cursor:pointer;font-size:.8em;flex-shrink:0;margin-left:6px">
        Show
      </button>
    </div>
    <div class="metric">
      <span class="metric-label" style="flex-shrink:0;width:68px">Battery</span>
      <select id="icon-bat" onchange="onIconChange('icon-bat','prev-bat')"
              style="flex:1;padding:4px 6px;background:var(--bg-secondary);
                     color:var(--text-color);border:1px solid var(--border-color);
                     border-radius:4px;font-size:.88em">
        <option value="">(none)</option>
      </select>
      <img id="prev-bat" src="" alt=""
           style="height:28px;width:auto;image-rendering:pixelated;margin-left:6px;
                  border-radius:2px;display:none">
      <button onclick="showIcon('icon-bat')"
              style="background:#444;color:#fff;border:none;padding:4px 10px;
                     border-radius:4px;cursor:pointer;font-size:.8em;flex-shrink:0;margin-left:6px">
        Show
      </button>
    </div>
    <div class="metric" style="border-bottom:none">
      <span class="metric-label" style="flex-shrink:0;width:68px">POCSAG</span>
      <select id="icon-poc" onchange="onIconChange('icon-poc','prev-poc')"
              style="flex:1;padding:4px 6px;background:var(--bg-secondary);
                     color:var(--text-color);border:1px solid var(--border-color);
                     border-radius:4px;font-size:.88em">
        <option value="">(none)</option>
      </select>
      <img id="prev-poc" src="" alt=""
           style="height:28px;width:auto;image-rendering:pixelated;margin-left:6px;
                  border-radius:2px;display:none">
      <button onclick="showIcon('icon-poc')"
              style="background:#444;color:#fff;border:none;padding:4px 10px;
                     border-radius:4px;cursor:pointer;font-size:.8em;flex-shrink:0;margin-left:6px">
        Show
      </button>
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

  <!-- Device Name -->
  <div class="card">
    <h3>Device Name</h3>
    <div style="margin-bottom:12px">
      <div style="font-size:.82em;color:var(--text-muted);margin-bottom:6px">Boot screen name (max 8 chars)</div>
      <div style="display:flex;align-items:center;gap:8px">
        <input type="text" id="boot-name" maxlength="8"
               style="flex:1;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:5px 8px;font-size:1em;text-transform:uppercase;letter-spacing:.1em"
               oninput="this.value=this.value.toUpperCase()">
        <button onclick="saveBootName()" class="btn btn-info">Save</button>
      </div>
      <div id="bootname-status" style="font-size:.78em;color:#4caf50;margin-top:4px;min-height:1em"></div>
    </div>
    <div style="margin-bottom:12px">
      <div style="font-size:.82em;color:var(--text-muted);margin-bottom:6px">mDNS hostname (<span id="mdns-preview">ulanzi</span>.local)</div>
      <div style="display:flex;align-items:center;gap:8px">
        <input type="text" id="mdns-name" maxlength="31"
               style="flex:1;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:5px 8px;font-size:1em"
               oninput="this.value=this.value.toLowerCase().replace(/[^a-z0-9-]/g,'');document.getElementById('mdns-preview').textContent=this.value||'ulanzi'">
        <button onclick="saveMdnsName()" class="btn btn-info">Save</button>
      </div>
      <div id="mdnsname-status" style="font-size:.78em;color:#4caf50;margin-top:4px;min-height:1em"></div>
    </div>
    <div>
      <div style="font-size:.82em;color:var(--text-muted);margin-bottom:6px">ArduinoOTA hostname (shown in IDE port list)</div>
      <div style="display:flex;align-items:center;gap:8px">
        <input type="text" id="ota-hostname" maxlength="31"
               style="flex:1;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:5px 8px;font-size:1em"
               oninput="this.value=this.value.toLowerCase().replace(/[^a-z0-9-]/g,'')">
        <button onclick="saveOtaHostname()" class="btn btn-info">Save</button>
      </div>
      <div id="otahostname-status" style="font-size:.78em;color:#4caf50;margin-top:4px;min-height:1em"></div>
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
    s.textContent='Rebooting…';
    fetch('/api/reboot',{method:'POST'}).catch(function(){});
    setTimeout(function(){s.textContent='Reconnecting…';},1500);
    setTimeout(function(){location.reload();},6000);
  });
}
function setDebugLog(val){
  fetch('/api/debug',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({debug:val})}).catch(function(){});
}
function saveOtaHostname(){
  var v=document.getElementById('ota-hostname').value.trim().toLowerCase().replace(/[^a-z0-9-]/g,'');
  if(v.length===0||v.length>31){
    document.getElementById('otahostname-status').textContent='1–31 chars (a-z, 0-9, -)';
    document.getElementById('otahostname-status').style.color='#f44';
    return;
  }
  fetch('/api/otahostname',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'name='+encodeURIComponent(v)})
  .then(function(r){return r.json();}).then(function(d){
    var s=document.getElementById('otahostname-status');
    s.textContent=d.ok?'Saved — takes effect after reboot':'Error: '+(d.error||'?');
    s.style.color=d.ok?'#4caf50':'#f44';
    setTimeout(function(){s.textContent='';},3000);
  }).catch(function(){});
}
function saveMdnsName(){
  var v=document.getElementById('mdns-name').value.trim().toLowerCase().replace(/[^a-z0-9-]/g,'');
  if(v.length===0||v.length>31){
    document.getElementById('mdnsname-status').textContent='1–31 chars (a-z, 0-9, -)';
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
        function(){s.textContent='Saved — reboot required';s.style.color='#ffc107';setTimeout(function(){s.textContent='';},4000);}
      );
    } else {
      s.textContent='Error: '+(d.error||'?');s.style.color='#f44';
    }
  }).catch(function(){});
}
function saveBootName(){
  var v=document.getElementById('boot-name').value.trim().toUpperCase();
  if(v.length===0||v.length>8){
    document.getElementById('bootname-status').textContent='1–8 characters required.';
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
    body:'t_thr_lo='+document.getElementById('t-thr-lo').value
        +'&t_thr_hi='+document.getElementById('t-thr-hi').value
        +'&t_lo='+encodeURIComponent(document.getElementById('t-col-lo').value)
        +'&t_mid='+encodeURIComponent(document.getElementById('t-col-mid').value)
        +'&t_hi='+encodeURIComponent(document.getElementById('t-col-hi').value)
        +'&h_thr_lo='+document.getElementById('h-thr-lo').value
        +'&h_thr_hi='+document.getElementById('h-thr-hi').value
        +'&h_lo='+encodeURIComponent(document.getElementById('h-col-lo').value)
        +'&h_mid='+encodeURIComponent(document.getElementById('h-col-mid').value)
        +'&h_hi='+encodeURIComponent(document.getElementById('h-col-hi').value)
        +'&b_thr_lo='+document.getElementById('b-thr-lo').value
        +'&b_thr_hi='+document.getElementById('b-thr-hi').value
        +'&b_lo='+encodeURIComponent(document.getElementById('b-col-lo').value)
        +'&b_mid='+encodeURIComponent(document.getElementById('b-col-mid').value)
        +'&b_hi='+encodeURIComponent(document.getElementById('b-col-hi').value)
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
    document.getElementById('tog-bz-boot').checked=d.buzzer_boot_en;
    document.getElementById('sld-bz-boot').value=d.buzzer_boot_vol;
    document.getElementById('bz-boot-num').textContent=d.buzzer_boot_vol;
    document.getElementById('tog-bz-poc').checked=d.buzzer_pocsag_en;
    document.getElementById('sld-bz-poc').value=d.buzzer_pocsag_vol;
    document.getElementById('bz-poc-num').textContent=d.buzzer_pocsag_vol;
    document.getElementById('tog-bz-clk').checked=d.buzzer_click_en;
    document.getElementById('sld-bz-clk').value=d.buzzer_click_vol;
    document.getElementById('bz-clk-num').textContent=d.buzzer_click_vol;
    var ri=d.rotate_interval||5;
    document.getElementById('tog-rot').checked=d.rotate_enabled;
    document.getElementById('sld-rot').value=ri;
    document.getElementById('rot-num').textContent=ri+'s';
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
    if(d.t_thr_lo!==undefined)document.getElementById('t-thr-lo').value=d.t_thr_lo;
    if(d.t_thr_hi!==undefined)document.getElementById('t-thr-hi').value=d.t_thr_hi;
    if(d.t_lo)document.getElementById('t-col-lo').value=d.t_lo;
    if(d.t_mid)document.getElementById('t-col-mid').value=d.t_mid;
    if(d.t_hi)document.getElementById('t-col-hi').value=d.t_hi;
    if(d.h_thr_lo!==undefined)document.getElementById('h-thr-lo').value=d.h_thr_lo;
    if(d.h_thr_hi!==undefined)document.getElementById('h-thr-hi').value=d.h_thr_hi;
    if(d.h_lo)document.getElementById('h-col-lo').value=d.h_lo;
    if(d.h_mid)document.getElementById('h-col-mid').value=d.h_mid;
    if(d.h_hi)document.getElementById('h-col-hi').value=d.h_hi;
    if(d.b_thr_lo!==undefined)document.getElementById('b-thr-lo').value=d.b_thr_lo;
    if(d.b_thr_hi!==undefined)document.getElementById('b-thr-hi').value=d.b_thr_hi;
    if(d.b_lo)document.getElementById('b-col-lo').value=d.b_lo;
    if(d.b_mid)document.getElementById('b-col-mid').value=d.b_mid;
    if(d.b_hi)document.getElementById('b-col-hi').value=d.b_hi;
  }).catch(function(){});
  fetch('/api/otahostname').then(function(r){return r.json();}).then(function(d){
    if(d.name)document.getElementById('ota-hostname').value=d.name;
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
  fetch('/api/screensaver').then(function(r){return r.json();}).then(function(d){
    document.getElementById('tog-ss').checked=d.enabled;
    document.getElementById('ss-timeout').value=d.timeout||60;
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
