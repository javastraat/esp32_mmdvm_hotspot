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
  "<style>" COMMON_CSS "</style>"
  THEME_INIT_SCRIPT
  "</head><body>"
  NAV_BAR
  NAV_LIVE_MODAL
  R"html(
<div class="container"><div class="grid">

  <!-- Storage -->
  <div class="card">
    <h3>Storage</h3>
    <div class="metric" style="border-bottom:none">
      <span class="metric-label">LittleFS</span>
      <span class="metric-value" id="fs-info">-</span>
    </div>
    <div style="height:8px;background:var(--border-color);border-radius:4px;margin-top:10px;overflow:hidden">
      <div id="fs-bar" style="height:100%;border-radius:4px;background:#00bcd4;width:0%;transition:width .5s"></div>
    </div>
  </div>

  <!-- LaMetric icon downloader -->
  <div class="card">
    <h3>Download LaMetric Icon
      <a href="https://developer.lametric.com/icons" target="_blank"
        style="font-size:.85em;font-weight:normal;color:#00bcd4;margin-left:8px;text-decoration:none">
        Browse &#8599;
      </a>
    </h3>
    <div style="display:flex;gap:10px;align-items:center;flex-wrap:wrap">
      <input type="text" id="icon-id" placeholder="Icon ID e.g. 2867"
        style="flex:1;min-width:80px;padding:6px 8px;border:1px solid var(--border-color);
               background:var(--bg-secondary);color:var(--text-color);border-radius:4px;font-size:.9em">
      <button onclick="previewIcon()" class="btn btn-secondary">Preview</button>
      <button onclick="downloadIcon()" class="btn btn-info">Save to /icons/</button>
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
                margin-bottom:8px;display:flex;align-items:center;gap:8px;
                border:1px solid var(--border-color)">
      <span class="metric-label" style="flex-shrink:0">Path:</span>
      <span id="fb-path"
        style="font-family:monospace;font-size:.9em;flex:1;overflow:hidden;
               text-overflow:ellipsis;white-space:nowrap">/</span>
    </div>

    <!-- Toolbar -->
    <div style="display:flex;flex-wrap:wrap;gap:6px;margin-bottom:10px">
      <button id="fb-up-btn" class="btn btn-secondary" onclick="fbGoUp()" disabled>&#8593; Up</button>
      <button class="btn btn-secondary" onclick="fbRefresh()">&#8635; Refresh</button>
      <button class="btn btn-success" onclick="fbMkdirToggle()">&#128193;+ Folder</button>
      <button class="btn btn-primary" onclick="fbUploadToggle()">&#8679; Upload</button>
    </div>

    <!-- Mkdir row (hidden) -->
    <div id="fb-mkdir-row"
         style="display:none;align-items:center;gap:6px;margin-bottom:8px">
      <input id="fb-mkdir-input" type="text" placeholder="New folder name"
        style="flex:1;padding:4px 8px;background:var(--bg-secondary);color:var(--text-color);
               border:1px solid var(--border-color);border-radius:4px;font-size:.88em"
        onkeydown="if(event.key==='Enter')fbMkdir()">
      <button class="btn btn-success" style="flex-shrink:0" onclick="fbMkdir()">Create</button>
      <button class="btn btn-secondary" style="flex-shrink:0" onclick="fbMkdirHide()">Cancel</button>
    </div>

    <!-- Upload row (hidden) -->
    <div id="fb-upload-row"
         style="display:none;align-items:center;gap:6px;margin-bottom:4px">
      <input type="file" id="fb-upload-file"
        style="flex:1;font-size:.82em;min-width:0;color:var(--text-color)">
      <button class="btn btn-success" style="flex-shrink:0" onclick="fbDoUpload()">Upload</button>
      <button class="btn btn-secondary" style="flex-shrink:0" onclick="fbUploadHide()">Cancel</button>
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
            <th style="text-align:right;padding:5px 2px;width:120px"></th>
          </tr>
        </thead>
        <tbody id="fb-body">
          <tr><td colspan="3" style="color:var(--text-muted);padding:8px;text-align:center">Loading...</td></tr>
        </tbody>
      </table>
    </div>
  </div>

</div></div>
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
        rows+='<td style="padding:5px 2px;text-align:right;white-space:nowrap">'
             +'<button onclick="fbRename(\''+esc+'\')"'
             +' class="btn btn-primary" style="padding:2px 8px;margin-right:4px">Ren</button>'
             +'<button onclick="fbDelete(\''+esc+'\')"'
             +' class="btn btn-danger" style="padding:2px 8px">Del</button></td>';
      }else{
        var isImg=/\.(jpe?g|gif|png|bmp)$/i.test(e.name);
        var nameCell=isImg
          ?'<span onclick="fbPreviewFile(\''+encodeURIComponent(e.path)+'\',\''+e.name+'\')"'
           +' style="cursor:pointer;color:#00bcd4;text-decoration:underline dotted">'+e.name+'</span>'
          :e.name;
        rows+='<td style="padding:5px 4px;font-family:monospace;word-break:break-all">&#128196; '+nameCell+'</td>';
        rows+='<td style="padding:5px 4px;text-align:right;font-family:monospace;'
             +'font-size:.82em;white-space:nowrap">'+fmtSize(e.size)+'</td>';
        rows+='<td style="padding:5px 2px;text-align:right;white-space:nowrap">'
             +'<a href="/api/fs/download?path='+encodeURIComponent(e.path)+'"'
             +' download="'+e.name+'"'
             +' class="btn btn-secondary" style="padding:2px 8px;margin-right:4px;text-decoration:none;display:inline-block">DL</a>'
             +'<button onclick="fbRename(\''+esc+'\')"'
             +' class="btn btn-primary" style="padding:2px 8px;margin-right:4px">Ren</button>'
             +'<button onclick="fbDelete(\''+esc+'\')"'
             +' class="btn btn-danger" style="padding:2px 8px">Del</button>'
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
  showConfirm('Delete: '+path+'?',function(){
    fetch('/api/fs/delete?path='+encodeURIComponent(path),{method:'POST'})
    .then(function(r){return r.text();})
    .then(function(msg){showAlert(msg);fbRefresh();loadFs();})
    .catch(function(){showAlert('Delete failed');});
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
  .then(function(msg){showAlert(msg);fbMkdirHide();fbRefresh();})
  .catch(function(){showAlert('Error creating folder');});
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
  if(!fi.files.length){showAlert('Select a file first');return;}
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
  st.textContent='Fetching\u2026';st.style.color='var(--text-muted)';
  // Fetch via ESP32 proxy (avoids CORS issues)
  fetch('/api/icons/proxy?id='+encodeURIComponent(id))
  .then(function(r){
    if(!r.ok)throw new Error('HTTP '+r.status);
    var ct=r.headers.get('content-type')||'';
    return r.blob().then(function(blob){return{blob:blob,ct:ct};});
  })
  .then(function(d){
    var blob=d.blob,ct=d.ct;
    if(ct.indexOf('gif')>=0){
      // GIF — upload as-is, animation preserved
      return _saveIconBlob(id+'.gif',blob,st);
    }
    // PNG or JPEG — convert to JPEG via canvas so TJpg_Decoder can display it
    st.textContent='Converting\u2026';st.style.color='var(--text-muted)';
    return new Promise(function(resolve,reject){
      var blobUrl=URL.createObjectURL(blob);
      var img=new Image();
      img.onload=function(){
        URL.revokeObjectURL(blobUrl);
        var canvas=document.createElement('canvas');
        canvas.width=img.naturalWidth;canvas.height=img.naturalHeight;
        canvas.getContext('2d').drawImage(img,0,0);
        canvas.toBlob(function(jpegBlob){
          if(!jpegBlob){reject(new Error('canvas conversion failed'));return;}
          resolve(_saveIconBlob(id+'.jpg',jpegBlob,st));
        },'image/jpeg',1.0);
      };
      img.onerror=function(){URL.revokeObjectURL(blobUrl);reject(new Error('image load failed'));};
      img.src=blobUrl;
    });
  })
  .catch(function(e){st.textContent='Error: '+e.message;st.style.color='#dc3545';});
}
function _saveIconBlob(filename,blob,st){
  st.textContent='Saving\u2026';st.style.color='var(--text-muted)';
  var fd=new FormData();fd.append('file',blob,filename);
  return fetch('/api/files/upload?dir=%2Ficons',{method:'POST',body:fd})
  .then(function(r){return r.json();})
  .then(function(d){
    if(d.ok){
      st.textContent='Saved: /icons/'+d.name.split('/').pop()+' ('+fmtSize(blob.size)+')';
      st.style.color='#28a745';fbRefresh();loadFs();
    }else{throw new Error(d.error||'save failed');}
  });
}
function fbPreviewFile(encodedPath,name){
  var prev=document.getElementById('icon-preview');
  var img=document.getElementById('icon-img');
  var st=document.getElementById('icon-status');
  img.onerror=function(){st.textContent='Cannot preview: '+name;st.style.color='#dc3545';prev.style.display='none';};
  img.onload=function(){prev.style.display='block';st.textContent=name;st.style.color='var(--text-muted)';};
  img.src='/api/fs/download?path='+encodedPath;
  prev.style.display='none';
  prev.scrollIntoView({behavior:'smooth',block:'nearest'});
}
function fbRename(path){
  var sl=path.lastIndexOf('/');
  var dir=sl>0?path.substring(0,sl):'/';
  var name=path.substring(sl+1);
  showModal(function(box,close){
    var h=document.createElement('h4');h.textContent='Rename: '+name;box.appendChild(h);
    var inp=document.createElement('input');inp.type='text';inp.value=name;
    box.appendChild(inp);
    var d=document.createElement('div');d.className='modal-buttons';
    var ok=document.createElement('button');ok.textContent='Rename';ok.className='btn btn-info';
    ok.onclick=function(){
      var newName=inp.value.trim();
      if(!newName||newName===name){close();return;}
      var newPath=(dir==='/')?'/'+newName:dir+'/'+newName;
      close();
      fetch('/api/fs/rename?from='+encodeURIComponent(path)+'&to='+encodeURIComponent(newPath),{method:'POST'})
      .then(function(r){return r.text();})
      .then(function(msg){showAlert(msg);fbRefresh();})
      .catch(function(){showAlert('Rename failed');});
    };
    var no=document.createElement('button');no.textContent='Cancel';no.className='btn btn-secondary';
    no.onclick=close;
    d.appendChild(ok);d.appendChild(no);box.appendChild(d);
    setTimeout(function(){inp.focus();inp.select();},50);
    inp.onkeydown=function(e){if(e.key==='Enter')ok.onclick();if(e.key==='Escape')no.onclick();};
  });
}
loadFs();
fbNavigate('/');
</script>
</body></html>
)html";
