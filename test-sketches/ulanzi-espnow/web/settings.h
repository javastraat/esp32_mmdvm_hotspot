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
  "<style>" COMMON_CSS "</style>"
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
      <input type="range" id="sld-bright" min="1" max="255" value="50" disabled
             oninput="document.getElementById('bright-num').textContent=this.value"
             onchange="onSliderChange()">
      <span class="bright-num" id="bright-num">50</span>
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
        <button onclick="testBuzzer('boot')" style="background:#444;color:#fff;border:none;padding:4px 10px;border-radius:4px;cursor:pointer;font-size:.8em;flex-shrink:0">Test</button>
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
        <button onclick="testBuzzer('pocsag')" style="background:#444;color:#fff;border:none;padding:4px 10px;border-radius:4px;cursor:pointer;font-size:.8em;flex-shrink:0">Test</button>
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
        <button onclick="testBuzzer('click')" style="background:#444;color:#fff;border:none;padding:4px 10px;border-radius:4px;cursor:pointer;font-size:.8em;flex-shrink:0">Test</button>
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
    <div style="display:flex;align-items:center;justify-content:flex-end;gap:10px;margin-top:10px">
      <span id="icon-status" style="font-size:.82em;color:var(--text-muted)"></span>
      <button onclick="saveIcons()"
              style="background:#00bcd4;color:#000;border:none;padding:6px 18px;
                     border-radius:4px;cursor:pointer;font-weight:bold;font-size:.88em">
        Save Icons
      </button>
    </div>
  </div>

</div></div>
)html"
  "<script>" COMMON_JS NAV_LIVE_JS "</script>"
  R"html(
<script>
function postBright(isAuto,level){
  fetch('/api/brightness',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'auto='+(isAuto?1:0)+'&level='+level
  }).catch(function(){});
}
function onAutoToggle(){
  var isAuto=document.getElementById('tog-auto').checked;
  document.getElementById('sld-bright').disabled=isAuto;
  document.getElementById('tog-lbl').textContent=isAuto?'Auto':'Manual';
  postBright(isAuto,document.getElementById('sld-bright').value);
}
function onSliderChange(){
  postBright(false,document.getElementById('sld-bright').value);
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
    document.getElementById('bright-num').textContent=d.brightness;
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
      populateIconSelect('icon-temp','prev-temp',files,d.temp||'');
      populateIconSelect('icon-hum', 'prev-hum', files,d.hum||'');
      populateIconSelect('icon-bat', 'prev-bat', files,d.bat||'');
      populateIconSelect('icon-poc', 'prev-poc', files,d.poc||'');
    }).catch(function(){});
  }).catch(function(){});
  fetch('/api/screensaver').then(function(r){return r.json();}).then(function(d){
    document.getElementById('tog-ss').checked=d.enabled;
    document.getElementById('ss-timeout').value=d.timeout||60;
    fetch('/api/fs/ls?path=/screensaver').then(function(r){return r.json();}).then(function(ls){
      var sel=document.getElementById('ss-file');
      (ls.entries||[]).filter(function(e){return !e.isDir&&/\.gif$/i.test(e.name);}).forEach(function(e){
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
