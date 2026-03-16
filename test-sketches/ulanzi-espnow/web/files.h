#pragma once
#include "styles.h"
#include "navigation.h"

// ── Files page (/files): LittleFS browser — list, upload, delete ──────────
static const char PAGE_FILES[] PROGMEM =
  "<!DOCTYPE html><html lang=\"en\">"
  "<head>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Ulanzi Files</title>"
  "<style>" COMMON_CSS "</style>"
  THEME_INIT_SCRIPT
  "</head><body>"
  NAV_BAR
  NAV_LIVE_MODAL
  R"html(
<div class="container">

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

  <div class="card" style="margin-bottom:15px">
    <h3>Upload File</h3>
    <div style="display:flex;gap:10px;align-items:center;flex-wrap:wrap">
      <input type="file" id="file-input" style="flex:1;min-width:120px;color:var(--text-color);font-size:.9em">
      <button onclick="doUpload()"
        style="background:#00bcd4;color:#000;border:none;padding:8px 20px;border-radius:4px;
               cursor:pointer;font-weight:bold;white-space:nowrap;font-size:.9em">
        Upload
      </button>
    </div>
    <div id="upload-status" style="margin-top:8px;font-size:.85em;color:var(--text-muted);min-height:1.2em"></div>
    <div id="upload-bar-wrap" style="display:none;height:6px;background:var(--border-color);border-radius:3px;margin-top:6px;overflow:hidden">
      <div id="upload-bar" style="height:100%;background:#00bcd4;width:0%;transition:width .2s"></div>
    </div>
  </div>

  <div class="card">
    <h3>Files</h3>
    <div id="file-list" style="min-height:40px">
      <span style="color:var(--text-muted);font-size:.9em">Loading...</span>
    </div>
  </div>

</div>
)html"
  "<script>" COMMON_JS NAV_LIVE_JS "</script>"
  R"html(
<script>
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
  }).catch(function(){
    document.getElementById('fs-info').textContent='unavailable';
  });
}
function loadFiles(){
  fetch('/api/files').then(function(r){return r.json();}).then(function(files){
    var el=document.getElementById('file-list');
    if(!files.length){
      el.innerHTML='<span style="color:var(--text-muted);font-size:.9em;padding:8px 0;display:block">No files</span>';
      return;
    }
    var h='<table style="width:100%;border-collapse:collapse;font-size:.88em">'
      +'<tr style="border-bottom:2px solid var(--border-color)">'
      +'<th style="text-align:left;padding:6px 4px;color:var(--text-muted);font-weight:bold">Name</th>'
      +'<th style="text-align:right;padding:6px 4px;color:var(--text-muted);font-weight:bold">Size</th>'
      +'<th style="width:56px"></th></tr>';
    files.forEach(function(f){
      h+='<tr style="border-bottom:1px solid var(--border-color)">'
        +'<td style="padding:7px 4px;font-family:monospace;word-break:break-all">'+f.name+'</td>'
        +'<td style="padding:7px 4px;text-align:right;font-family:monospace;white-space:nowrap">'+fmtSize(f.size)+'</td>'
        +'<td style="padding:7px 4px;text-align:right">'
        +'<button onclick="doDelete(\''+f.name+'\')" '
        +'style="background:#dc3545;color:#fff;border:none;padding:3px 10px;'
        +'border-radius:4px;cursor:pointer;font-size:.8em">Del</button></td></tr>';
    });
    h+='</table>';
    el.innerHTML=h;
  }).catch(function(){
    document.getElementById('file-list').innerHTML=
      '<span style="color:#dc3545;font-size:.9em">Error loading files</span>';
  });
}
function doDelete(name){
  if(!confirm('Delete '+name+'?'))return;
  var fd=new URLSearchParams();fd.append('name',name);
  fetch('/api/files/delete',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:fd.toString()})
  .then(function(r){return r.json();})
  .then(function(d){if(d.ok){loadFiles();loadFs();}else alert('Delete failed');})
  .catch(function(){alert('Delete failed');});
}
function doUpload(){
  var inp=document.getElementById('file-input');
  var st=document.getElementById('upload-status');
  var bar=document.getElementById('upload-bar');
  var wrap=document.getElementById('upload-bar-wrap');
  if(!inp.files.length){st.textContent='Select a file first.';return;}
  var fd=new FormData();
  fd.append('file',inp.files[0],inp.files[0].name);
  st.textContent='Uploading...';
  st.style.color='var(--text-muted)';
  wrap.style.display='block';
  bar.style.width='5%';
  var xhr=new XMLHttpRequest();
  xhr.open('POST','/api/files/upload');
  xhr.upload.onprogress=function(e){
    if(e.lengthComputable)bar.style.width=Math.round(e.loaded*100/e.total)+'%';
  };
  xhr.onload=function(){
    wrap.style.display='none';
    try{
      var d=JSON.parse(xhr.responseText);
      if(d.ok){
        st.textContent='Uploaded: '+d.name;
        st.style.color='#28a745';
        inp.value='';
        loadFiles();loadFs();
      }else{
        st.textContent='Error: '+(d.error||'upload failed');
        st.style.color='#dc3545';
      }
    }catch(e){st.textContent='Upload failed';st.style.color='#dc3545';}
  };
  xhr.onerror=function(){
    wrap.style.display='none';
    st.textContent='Upload failed';st.style.color='#dc3545';
  };
  xhr.send(fd);
}
loadFs();
loadFiles();
</script>
</body></html>
)html";
