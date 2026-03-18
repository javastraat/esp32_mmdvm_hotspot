#pragma once
#include "styles.h"
#include "navigation.h"

// ── MQTT page (/mqtt): broker settings + HA auto-discovery config ─────────────
static const char PAGE_MQTT[] PROGMEM =
  "<!DOCTYPE html><html lang=\"en\">"
  "<head>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Ulanzi MQTT</title>"
  "<style>" COMMON_CSS "</style>"
  THEME_INIT_SCRIPT
  "</head><body>"
  NAV_BAR
  NAV_LIVE_MODAL
  R"html(
<div class="container"><div class="grid">

  <!-- Status -->
  <div class="card">
    <h3>Connection</h3>
    <div class="metric">
      <span class="metric-label">Status</span>
      <span id="mqtt-status" class="metric-value">-</span>
    </div>
    <div class="metric">
      <span class="metric-label">Broker</span>
      <span id="mqtt-broker-disp" class="metric-value" style="font-family:monospace">-</span>
    </div>
    <div class="metric">
      <span class="metric-label">Node ID</span>
      <span id="mqtt-node-disp" class="metric-value" style="font-family:monospace">-</span>
    </div>
    <div class="metric">
      <span class="metric-label">Topics base</span>
      <span id="mqtt-topics-disp" class="metric-value" style="font-family:monospace;font-size:.8em">-</span>
    </div>
    <div class="metric" style="border-bottom:none">
      <span class="metric-label">HA Device Name</span>
      <span id="mqtt-haname-disp" class="metric-value" style="font-family:monospace">-</span>
    </div>
  </div>



  <!-- Settings -->
  <div class="card">
    <h3>Settings</h3>

    <div class="metric">
      <div>
        <span class="metric-label">Enable MQTT</span>
        <div style="font-size:.75em;color:var(--text-muted);margin-top:2px">Master on/off. Changes take effect on Save.</div>
      </div>
      <label class="switch" style="margin-left:12px;flex-shrink:0">
        <input type="checkbox" id="mqtt-en">
        <span class="slider"></span>
      </label>
    </div>

    <div style="margin-top:14px;display:flex;flex-direction:column;gap:10px">

      <div>
        <label class="metric-label" for="mqtt-broker-in">Broker Host / IP</label>
        <input type="text" id="mqtt-broker-in" placeholder="192.168.1.x or hostname"
          style="width:100%;margin-top:4px;padding:6px 9px;background:var(--bg-secondary);
                 color:var(--text-color);border:1px solid var(--border-color);
                 border-radius:4px;font-size:.9em;box-sizing:border-box">
      </div>

      <div>
        <label class="metric-label" for="mqtt-port-in">Port</label>
        <input type="number" id="mqtt-port-in" min="1" max="65535" value="1883"
          style="width:100%;margin-top:4px;padding:6px 9px;background:var(--bg-secondary);
                 color:var(--text-color);border:1px solid var(--border-color);
                 border-radius:4px;font-size:.9em;box-sizing:border-box">
      </div>

      <div>
        <label class="metric-label" for="mqtt-user-in">Username <span style="font-weight:normal">(optional)</span></label>
        <input type="text" id="mqtt-user-in" placeholder="leave empty if no auth"
          autocomplete="off"
          style="width:100%;margin-top:4px;padding:6px 9px;background:var(--bg-secondary);
                 color:var(--text-color);border:1px solid var(--border-color);
                 border-radius:4px;font-size:.9em;box-sizing:border-box">
      </div>

      <div>
        <label class="metric-label" for="mqtt-pass-in">Password</label>
        <div style="display:flex;gap:6px;margin-top:4px">
          <input type="password" id="mqtt-pass-in" placeholder="leave empty if no auth"
            autocomplete="new-password"
            style="flex:1;padding:6px 9px;background:var(--bg-secondary);
                   color:var(--text-color);border:1px solid var(--border-color);
                   border-radius:4px;font-size:.9em;box-sizing:border-box">
          <button class="btn btn-secondary" style="flex-shrink:0;padding:6px 10px"
            onclick="togglePassVis()">&#128065;</button>
        </div>
      </div>

      <div>
        <label class="metric-label" for="mqtt-node-in">Node ID <span style="font-weight:normal">(MQTT topic prefix)</span></label>
        <input type="text" id="mqtt-node-in" placeholder="ulanzi"
          oninput="updatePreview()"
          style="width:100%;margin-top:4px;padding:6px 9px;background:var(--bg-secondary);
                 color:var(--text-color);border:1px solid var(--border-color);
                 border-radius:4px;font-size:.9em;box-sizing:border-box">
        <div id="node-preview" style="font-size:.75em;color:var(--text-muted);margin-top:3px;font-family:monospace"></div>
        <div style="font-size:.75em;color:var(--text-muted);margin-top:2px">Used in all MQTT topics and unique entity IDs. Short, lowercase, no spaces.</div>
      </div>


    </div>
    <div style="margin-top:16px;display:flex;gap:8px;flex-wrap:wrap">
      <button class="btn btn-info" style="flex:1" onclick="saveMqtt()">Save &amp; Reboot</button>
    </div>
    <div id="mqtt-save-status" style="font-size:.78em;color:#aaa;margin-top:6px;min-height:1em"></div>
  </div> <!-- End Settings card -->

  <!-- Home Assistant Integration Card (Card 4) -->
  <div class="card" style="margin-top:0">
    <h3>Home Assistant Integration</h3>
    <div class="metric" style="border-bottom:none">
      <div>
        <span class="metric-label">HA Auto-Discovery</span>
        <div style="font-size:.75em;color:var(--text-muted);margin-top:2px">Publishes config payloads so HA discovers entities automatically.</div>
      </div>
      <label class="switch" style="margin-left:12px;flex-shrink:0">
        <input type="checkbox" id="mqtt-disc" checked>
        <span class="slider"></span>
      </label>
    </div>
    <div style="margin-top:10px">
      <label class="metric-label" for="mqtt-ha-name-in">HA Device Name</label>
      <input type="text" id="mqtt-ha-name-in" placeholder="(defaults to Node ID)"
        oninput="updatePreview()"
        style="width:100%;margin-top:4px;padding:6px 9px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;font-size:.9em;box-sizing:border-box">
      <div id="ha-name-preview" style="font-size:.75em;color:var(--text-muted);margin-top:3px;font-family:monospace"></div>
      <div style="font-size:.75em;color:var(--text-muted);margin-top:2px">Shown in Home Assistant device registry and used as entity ID prefix. Leave empty to use Node ID.</div>
    </div>
    <div style="margin-top:10px">
      <label class="metric-label" for="mqtt-prefix-in">Discovery Prefix</label>
      <input type="text" id="mqtt-prefix-in" value="homeassistant"
        style="width:100%;margin-top:4px;padding:6px 9px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;font-size:.9em;box-sizing:border-box">
      <div style="font-size:.75em;color:var(--text-muted);margin-top:3px">Must match HA's MQTT discovery prefix (default: homeassistant).</div>
    </div>
    <div style="margin-top:16px;display:flex;gap:8px;flex-wrap:wrap">
      <button class="btn btn-secondary" onclick="sendDiscovery()" id="disc-btn">Re-send Discovery</button>
    </div>
    <div id="mqtt-save-status" style="font-size:.78em;color:#aaa;margin-top:6px;min-height:1em"></div>
  </div>

</div></div>
)html"
  "<script>" COMMON_JS NAV_LIVE_JS "</script>"
  R"html(
<script>
function togglePassVis(){
  var i=document.getElementById('mqtt-pass-in');
  i.type=i.type==='password'?'text':'password';
}
function updatePreview(){
  var node=(document.getElementById('mqtt-node-in').value.trim()||'ulanzi');
  var haName=(document.getElementById('mqtt-ha-name-in').value.trim()||node);
  var haSlug=haName.toLowerCase().replace(/[^a-z0-9]+/g,'_').replace(/^_|_$/g,'');
  document.getElementById('node-preview').textContent=node+'/sensor/temperature/state  etc.';
  document.getElementById('ha-name-preview').textContent='sensor.'+haSlug+'_temperature  etc.';
}

function saveMqtt(){
  var broker=document.getElementById('mqtt-broker-in').value.trim();
  var port=parseInt(document.getElementById('mqtt-port-in').value)||1883;
  var user=document.getElementById('mqtt-user-in').value;
  var pass=document.getElementById('mqtt-pass-in').value;
  var node=document.getElementById('mqtt-node-in').value.trim().replace(/[^a-zA-Z0-9_-]/g,'');
  var prefix=document.getElementById('mqtt-prefix-in').value.trim();
  var en=document.getElementById('mqtt-en').checked;
  var disc=document.getElementById('mqtt-disc').checked;
  var haName=document.getElementById('mqtt-ha-name-in').value.trim();

  if(en && !broker){
    showAlert('Enter a broker host or IP address.');
    return;
  }
  if(!node){
    showAlert('Node ID cannot be empty.');
    return;
  }

  var st=document.getElementById('mqtt-save-status');
  st.textContent='Saving...';st.style.color='var(--text-muted)';

  var body='enabled='+(en?'1':'0')
    +'&broker='+encodeURIComponent(broker)
    +'&port='+port
    +'&user='+encodeURIComponent(user)
    +'&pass='+encodeURIComponent(pass)
    +'&node='+encodeURIComponent(node)
    +'&prefix='+encodeURIComponent(prefix)
    +'&discovery='+(disc?'1':'0')
    +'&ha_name='+encodeURIComponent(haName);

  fetch('/api/mqtt',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})
  .then(function(r){return r.json();})
  .then(function(d){
    if(!d.ok){
      st.style.color='#dc3545';st.textContent='Error saving';
      return;
    }
    st.style.color='#28a745';st.textContent='Saved.';
    showConfirm('Settings saved. Reboot now to apply?',function(){
      st.textContent='Rebooting\u2026';st.style.color='var(--text-muted)';
      fetch('/api/reboot',{method:'POST'}).catch(function(){});
      setTimeout(function(){st.textContent='Reconnecting\u2026';},2000);
      setTimeout(function(){location.reload();},7000);
    });
  })
  .catch(function(){st.style.color='#dc3545';st.textContent='Save failed';});
}
function sendDiscovery(){
  var btn=document.getElementById('disc-btn');
  btn.disabled=true;
  fetch('/api/mqtt/discovery',{method:'POST'})
  .then(function(r){return r.json();})
  .then(function(d){
    document.getElementById('mqtt-save-status').textContent=d.ok?'Discovery sent':'Not connected';
    document.getElementById('mqtt-save-status').style.color=d.ok?'#28a745':'#dc3545';
  })
  .catch(function(){})
  .finally(function(){btn.disabled=false;});
}
// pollStatus — updates only the connection status card (runs every 5 s, safe while editing)
function pollStatus(){
  fetch('/api/mqtt')
  .then(function(r){return r.json();})
  .then(function(d){
    var connected=d.connected;
    var enabled=d.enabled;
    var badge='<span class="badge '+(connected?'badge-success':enabled?'badge-danger':'badge-warning')+'">'
      +(connected?'Connected':enabled?'Disconnected':'Disabled')+'</span>';
    document.getElementById('mqtt-status').innerHTML=badge;
    document.getElementById('mqtt-broker-disp').textContent=d.broker||'—';
    var node=d.node_id||'ulanzi';
    document.getElementById('mqtt-node-disp').textContent=node;
    document.getElementById('mqtt-topics-disp').textContent=node+'/sensor/…';
    document.getElementById('mqtt-haname-disp').textContent=d.ha_name||node;
  })
  .catch(function(){});
}
// loadForm — populates form fields once on page load only
function loadForm(){
  fetch('/api/mqtt')
  .then(function(r){return r.json();})
  .then(function(d){
    var node=d.node_id||'ulanzi';
    document.getElementById('mqtt-en').checked=d.enabled;
    document.getElementById('mqtt-broker-in').value=d.broker||'';
    document.getElementById('mqtt-port-in').value=d.port||1883;
    document.getElementById('mqtt-user-in').value=d.user||'';
    document.getElementById('mqtt-pass-in').value=d.pass||'';
    document.getElementById('mqtt-node-in').value=node;
    document.getElementById('mqtt-prefix-in').value=d.prefix||'homeassistant';
    document.getElementById('mqtt-disc').checked=d.discovery;
    document.getElementById('mqtt-ha-name-in').value=d.ha_name||'';
    updatePreview();
  })
  .catch(function(){});
}
loadForm();
pollStatus();
setInterval(pollStatus,5000);
</script>
</body></html>
)html";
