#pragma once
#include "styles.h"
#include "navigation.h"

// ── Files page (/files): storage info, LittleFS browser, LaMetric icon downloader ──
static const char PAGE_FILES[] PROGMEM =
  "<!DOCTYPE html><html lang=\"en\">"
  "<head>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Ulanzi Files</title>"
  "<style>" COMMON_CSS
  ".fb-modal-ov{position:fixed;top:0;left:0;width:100%;height:100%;"
  "background:rgba(0,0,0,.65);z-index:9000;display:flex;align-items:center;justify-content:center}"
  ".fb-modal-box{background:var(--container-bg);border:1px solid var(--border-color);"
  "border-radius:8px;padding:22px 24px;min-width:240px;max-width:380px;text-align:center;"
  "box-shadow:0 4px 20px rgba(0,0,0,.5)}"
  ".fb-modal-box h4{margin-bottom:4px;font-size:.95em;line-height:1.45;word-break:break-all}"
  ".fb-btn-ok{background:#00bcd4;color:#000;border:none;padding:7px 22px;border-radius:4px;"
  "cursor:pointer;font-weight:bold;font-size:.9em}"
  ".fb-btn-cancel{background:#555;color:#fff;border:none;padding:7px 18px;border-radius:4px;"
  "cursor:pointer;font-size:.9em}"
  ".fb-tbtn{border:none;padding:4px 12px;border-radius:4px;cursor:pointer;font-size:.82em;font-weight:bold}"
  "</style>"
  THEME_INIT_SCRIPT
  "</head><body>"
  NAV_BAR
  NAV_LIVE_MODAL
  R"html(
<div class="container">

  <!-- Storage -->
  <div class="card" style="margin-bottom:15px">
    <h3>Storage</h3>
    <div class="metric">
      <span class="metric-label">LittleFS</span>
      <span class="metric-value" id="fs-info">-</span>
    </div>
    <div style="height:8px;background:var(--border-color);border-radius:4px;margin-top:10px;overflow:hidden">
      <div id="fs-bar" style="height:100%;border-radius:4px;background:#00bcd4;width:0%;transition:width .5s"></div>
    </div>
  </div>

  <!-- LaMetric icon downloader -->
  <div class="card" style="margin-bottom:15px">
    <h3>Download LaMetric Icon</h3>
    <div style="display:flex;gap:10px;align-items:center;flex-wrap:wrap">
      <input type="text" id="icon-id" placeholder="Icon ID e.g. 2867"
        style="flex:1;min-width:80px;padding:6px 8px;border:1px solid var(--border-color);
               background:var(--card-bg);color:var(--text-color);border-radius:4px;font-size:.9em">
      <button onclick="previewIcon()"
        style="background:var(--card-bg);color:var(--text-color);border:1px solid var(--border-color);
               padding:8px 16px;border-radius:4px;cursor:pointer;font-size:.9em;white-space:nowrap">
        Preview
      </button>
      <button onclick="downloadIcon()"
        style="background:#00bcd4;color:#000;border:none;padding:8px 20px;border-radius:4px;
               cursor:pointer;font-weight:bold;white-space:nowrap;font-size:.9em">
        Save to /icons/
      </button>
    </div>
    <div id="icon-status" style="margin-top:8px;font-size:.85em;color:var(--text-muted);min-height:1.2em"></div>
    <div id="icon-preview" style="display:none;margin-top:8px">
      <img id="icon-img"
        style="image-rendering:pixelated;width:64px;height:64px;background:#000;
               border-radius:4px;object-fit:contain;display:block">
    </div>
  </div>

  <!-- File browser -->
  <div class="card">
    <h3>Files</h3>

    <!-- Path bar -->
    <div style="background:var(--bg-secondary);border-radius:4px;padding:5px 10px;
                margin-bottom:6px;display:flex;align-items:center;gap:8px">
      <span style="color:var(--text-muted);font-size:.82em;flex-shrink:0">Path:</span>
      <span id="fb-path"
        style="font-family:monospace;font-size:.9em;flex:1;overflow:hidden;
               text-overflow:ellipsis;white-space:nowrap">/</span>
    </div>

    <!-- Toolbar -->
    <div style="display:flex;flex-wrap:wrap;gap:6px;margin-bottom:8px">
      <button id="fb-up-btn" class="fb-tbtn" style="background:#555;color:#fff"
              onclick="fbGoUp()" disabled>&#8593; Up</button>
      <button class="fb-tbtn" style="background:#444;color:#fff"
              onclick="fbRefresh()">&#8635; Refresh</button>
      <button class="fb-tbtn" style="background:#2e7d32;color:#fff"
              onclick="fbMkdirToggle()">&#128193;+ Folder</button>
      <button class="fb-tbtn" style="background:#1565c0;color:#fff"
              onclick="fbUploadToggle()">&#8679; Upload</button>
    </div>

    <!-- Mkdir row (hidden) -->
    <div id="fb-mkdir-row"
         style="display:none;align-items:center;gap:6px;margin-bottom:8px">
      <input id="fb-mkdir-input" type="text" placeholder="New folder name"
        style="flex:1;padding:4px 8px;background:var(--bg-secondary);color:var(--text-color);
               border:1px solid var(--border-color);border-radius:4px;font-size:.88em"
        onkeydown="if(event.key==='Enter')fbMkdir()">
      <button class="fb-tbtn" style="background:#2e7d32;color:#fff;flex-shrink:0"
              onclick="fbMkdir()">Create</button>
      <button class="fb-tbtn" style="background:#555;color:#fff;flex-shrink:0"
              onclick="fbMkdirHide()">Cancel</button>
    </div>

    <!-- Upload row (hidden) -->
    <div id="fb-upload-row"
         style="display:none;align-items:center;gap:6px;margin-bottom:4px">
      <input type="file" id="fb-upload-file"
        style="flex:1;font-size:.82em;min-width:0;color:var(--text-color)">
      <button class="fb-tbtn" style="background:#2e7d32;color:#fff;flex-shrink:0"
              onclick="fbDoUpload()">Upload</button>
      <button class="fb-tbtn" style="background:#555;color:#fff;flex-shrink:0"
              onclick="fbUploadHide()">Cancel</button>
    </div>
    <div id="fb-upload-status"
         style="display:none;font-size:.82em;margin-bottom:6px;padding:0 2px"></div>

    <!-- File table -->
    <div style="overflow-x:auto">
      <table style="width:100%;border-collapse:collapse;font-size:.88em">
        <thead>
          <tr style="border-bottom:2px solid var(--border-color)">
            <th style="text-align:left;padding:5px 4px;color:var(--text-muted);font-weight:bold">Name</th>
            <th style="text-align:right;padding:5px 4px;color:var(--text-muted);font-weight:bold">Size</th>
            <th style="text-align:right;padding:5px 2px;width:90px"></th>
          </tr>
        </thead>
        <tbody id="fb-body">
          <tr><td colspan="3" style="color:var(--text-muted);padding:8px;text-align:center">Loading...</td></tr>
        </tbody>
      </table>
    </div>
  </div>

</div>
)html"
  "<script>" COMMON_JS NAV_LIVE_JS "</script>"
  R"html(
<script>
var fbPath='/';
function fmtSize(b){
  if(b<1024)return b+' B';
  if(b<1048576)return (b/1024).toFixed(1)+' KB';
  return (b/1048576).toFixed(2)+' MB';
}
function loadFs(){
  fetch('/api/fs').then(function(r){return r.json();}).then(function(d){
    var pct=d.total>0?Math.round(d.used*100/d.total):0;
    document.getElementById('fs-info').textContent=
      fmtSize(d.used)+' / '+fmtSize(d.total)+' \u2014 '+pct+'% used';
    var bar=document.getElementById('fs-bar');
    bar.style.width=pct+'%';
    bar.style.background=pct>85?'#dc3545':pct>60?'#ffc107':'#00bcd4';
  }).catch(function(){document.getElementById('fs-info').textContent='unavailable';});
}
function fbGoUp(){
  if(fbPath==='/')return;
  var p=fbPath.endsWith('/')?fbPath.slice(0,-1):fbPath;
  var i=p.lastIndexOf('/');
  fbNavigate(i<=0?'/':p.substring(0,i));
}
function fbRefresh(){fbNavigate(fbPath);}
function fbNavigate(path){
  fbPath=path;
  document.getElementById('fb-path').textContent=path;
  document.getElementById('fb-up-btn').disabled=(path==='/');
  var tbody=document.getElementById('fb-body');
  tbody.innerHTML='<tr><td colspan="3" style="color:var(--text-muted);padding:8px;text-align:center">Loading...</td></tr>';
  fetch('/api/fs/ls?path='+encodeURIComponent(path))
  .then(function(r){return r.json();})
  .then(function(d){
    var entries=d.entries||[];
    entries.sort(function(a,b){
      if(a.isDir!==b.isDir)return a.isDir?-1:1;
      return a.name.localeCompare(b.name);
    });
    var rows='';
    entries.forEach(function(e){
      var esc=e.path.replace(/\\/g,'\\\\').replace(/'/g,"\\'");
      rows+='<tr style="border-bottom:1px solid var(--border-color)">';
      if(e.isDir){
        rows+='<td style="padding:5px 4px;cursor:pointer;color:#00bcd4;font-family:monospace"'
             +' onclick="fbNavigate(\''+esc+'\')">&#128193; '+e.name+'/</td>';
        rows+='<td style="padding:5px 4px;color:var(--text-muted);text-align:right;font-size:.82em">dir</td>';
        rows+='<td style="padding:5px 2px;text-align:right">'
             +'<button onclick="fbDelete(\''+esc+'\')"'
             +' style="background:#dc3545;color:#fff;border:none;padding:2px 8px;'
             +'border-radius:4px;cursor:pointer;font-size:.8em">Del</button></td>';
      }else{
        rows+='<td style="padding:5px 4px;font-family:monospace;word-break:break-all">&#128196; '+e.name+'</td>';
        rows+='<td style="padding:5px 4px;text-align:right;font-family:monospace;'
             +'font-size:.82em;white-space:nowrap">'+fmtSize(e.size)+'</td>';
        rows+='<td style="padding:5px 2px;text-align:right;white-space:nowrap">'
             +'<a href="/api/fs/download?path='+encodeURIComponent(e.path)+'"'
             +' download="'+e.name+'"'
             +' style="background:#555;color:#fff;text-decoration:none;padding:2px 8px;'
             +'border-radius:4px;font-size:.8em;margin-right:4px;display:inline-block">DL</a>'
             +'<button onclick="fbDelete(\''+esc+'\')"'
             +' style="background:#dc3545;color:#fff;border:none;padding:2px 8px;'
             +'border-radius:4px;cursor:pointer;font-size:.8em">Del</button>'
             +'</td>';
      }
      rows+='</tr>';
    });
    if(!rows)rows='<tr><td colspan="3" style="color:var(--text-muted);padding:8px;text-align:center">Empty</td></tr>';
    tbody.innerHTML=rows;
  })
  .catch(function(){
    tbody.innerHTML='<tr><td colspan="3" style="color:#dc3545;padding:8px">Error loading directory</td></tr>';
  });
}
function fbDelete(path){
  fbConfirm('Delete: '+path+'?',function(){
    fetch('/api/fs/delete?path='+encodeURIComponent(path),{method:'POST'})
    .then(function(r){return r.text();})
    .then(function(msg){fbAlert(msg);fbRefresh();loadFs();})
    .catch(function(){fbAlert('Delete failed');});
  });
}
function fbMkdirToggle(){
  fbUploadHide();
  var row=document.getElementById('fb-mkdir-row');
  row.style.display=row.style.display==='flex'?'none':'flex';
  if(row.style.display==='flex'){
    document.getElementById('fb-mkdir-input').value='';
    document.getElementById('fb-mkdir-input').focus();
  }
}
function fbMkdirHide(){document.getElementById('fb-mkdir-row').style.display='none';}
function fbMkdir(){
  var name=document.getElementById('fb-mkdir-input').value.trim();
  if(!name)return;
  var path=(fbPath==='/')?'/'+name:fbPath+'/'+name;
  fetch('/api/fs/mkdir?path='+encodeURIComponent(path),{method:'POST'})
  .then(function(r){return r.text();})
  .then(function(msg){fbAlert(msg);fbMkdirHide();fbRefresh();})
  .catch(function(){fbAlert('Error creating folder');});
}
function fbUploadToggle(){
  fbMkdirHide();
  var row=document.getElementById('fb-upload-row');
  row.style.display=row.style.display==='flex'?'none':'flex';
  document.getElementById('fb-upload-status').style.display='none';
  if(row.style.display==='flex')document.getElementById('fb-upload-file').value='';
}
function fbUploadHide(){
  document.getElementById('fb-upload-row').style.display='none';
  document.getElementById('fb-upload-status').style.display='none';
}
function fbDoUpload(){
  var fi=document.getElementById('fb-upload-file');
  if(!fi.files.length){fbAlert('Select a file first');return;}
  var file=fi.files[0];
  var fd=new FormData();fd.append('file',file,file.name);
  var st=document.getElementById('fb-upload-status');
  st.style.display='block';st.style.color='var(--text-muted)';st.textContent='Uploading...';
  var xhr=new XMLHttpRequest();
  xhr.upload.onprogress=function(e){
    if(e.lengthComputable)st.textContent='Uploading '+Math.round(e.loaded*100/e.total)+'%';
  };
  xhr.onload=function(){
    var ok=xhr.status===200;
    st.style.color=ok?'#28a745':'#dc3545';
    try{
      var d=JSON.parse(xhr.responseText);
      st.textContent=ok?'Uploaded: '+d.name:'Error: '+(d.error||'failed');
    }catch(e){st.textContent=ok?'Uploaded':'Failed';}
    if(ok){fi.value='';fbRefresh();loadFs();}
  };
  xhr.onerror=function(){st.style.color='#dc3545';st.textContent='Network error';};
  xhr.open('POST','/api/files/upload?dir='+encodeURIComponent(fbPath));
  xhr.send(fd);
}
function previewIcon(){
  var id=document.getElementById('icon-id').value.trim();
  var st=document.getElementById('icon-status');
  var prev=document.getElementById('icon-preview');
  var img=document.getElementById('icon-img');
  if(!id){st.textContent='Enter an icon ID.';st.style.color='#dc3545';return;}
  st.textContent='';
  img.onerror=function(){prev.style.display='none';st.textContent='Icon not found.';st.style.color='#dc3545';};
  img.onload=function(){prev.style.display='block';st.textContent='';};
  img.src='https://developer.lametric.com/content/apps/icon_thumbs/'+id;
}
function downloadIcon(){
  var id=document.getElementById('icon-id').value.trim();
  var st=document.getElementById('icon-status');
  if(!id){st.textContent='Enter an icon ID.';st.style.color='#dc3545';return;}
  st.textContent='Downloading\u2026';st.style.color='var(--text-muted)';
  var fd=new URLSearchParams();fd.append('id',id);
  fetch('/api/icons/download',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},body:fd.toString()})
  .then(function(r){return r.json();})
  .then(function(d){
    if(d.ok){
      st.textContent='Saved: '+d.name+' ('+fmtSize(d.size)+')';
      st.style.color='#28a745';
      fbRefresh();loadFs();
    }else{
      st.textContent='Error: '+(d.error||'download failed');
      st.style.color='#dc3545';
    }
  })
  .catch(function(){st.textContent='Request failed';st.style.color='#dc3545';});
}
// ── Modal helpers ──────────────────────────────────────────────
function fbAlert(msg){
  var ov=document.createElement('div');ov.className='fb-modal-ov';
  var box=document.createElement('div');box.className='fb-modal-box';
  var h=document.createElement('h4');h.textContent=msg;box.appendChild(h);
  var d=document.createElement('div');d.style.marginTop='14px';
  var btn=document.createElement('button');btn.textContent='OK';btn.className='fb-btn-ok';
  btn.onclick=function(){document.body.removeChild(ov);};
  d.appendChild(btn);box.appendChild(d);ov.appendChild(box);document.body.appendChild(ov);
}
function fbConfirm(msg,onYes){
  var ov=document.createElement('div');ov.className='fb-modal-ov';
  var box=document.createElement('div');box.className='fb-modal-box';
  var h=document.createElement('h4');h.textContent=msg;box.appendChild(h);
  var d=document.createElement('div');
  d.style.cssText='display:flex;gap:10px;justify-content:center;margin-top:14px';
  var yes=document.createElement('button');yes.textContent='Yes';yes.className='fb-btn-ok';
  yes.onclick=function(){document.body.removeChild(ov);onYes();};
  var no=document.createElement('button');no.textContent='Cancel';no.className='fb-btn-cancel';
  no.onclick=function(){document.body.removeChild(ov);};
  d.appendChild(yes);d.appendChild(no);box.appendChild(d);ov.appendChild(box);document.body.appendChild(ov);
}
loadFs();
fbNavigate('/');
</script>
</body></html>
)html";
