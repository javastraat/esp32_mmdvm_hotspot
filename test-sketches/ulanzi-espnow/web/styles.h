#pragma once
// Shared CSS, theme-init script, and common JS — included by every page header.
// Usage: embed COMMON_CSS inside a <style> tag, THEME_INIT_SCRIPT in <head>,
//        and emit "<script>" COMMON_JS "</script>" once per page.

// ── CSS ────────────────────────────────────────────────────────────────────
#define COMMON_CSS \
  ":root{--bg-color:#f0f0f0;--container-bg:white;--text-color:#333;--text-primary:#333;" \
  "--text-muted:#6c757d;--border-color:#dee2e6;--card-bg:#f8f9fa;--bg-secondary:#f8f9fa;" \
  "--info-bg:#e7f3ff;--link-color:#007bff;--link-hover-color:#0056b3;" \
  "--topnav-bg:#333;--topnav-text:#f2f2f2;--topnav-hover:#ddd;--topnav-hover-text:black}" \
  "[data-theme='dark']{--bg-color:#1a1a1a;--container-bg:#2d2d2d;--text-color:#ffffff;" \
  "--text-primary:#ffffff;--text-muted:#adb5bd;--border-color:#555;--card-bg:#3a3a3a;" \
  "--bg-secondary:#3a3a3a;--info-bg:#1e3a5f;--link-color:#4da6ff;--link-hover-color:#66b3ff;" \
  "--topnav-bg:#000;--topnav-text:#f2f2f2;--topnav-hover:#444;--topnav-hover-text:#ffffff}" \
  "*{box-sizing:border-box;margin:0;padding:0}" \
  "body{font-family:Arial,sans-serif;background:var(--bg-color);color:var(--text-color);" \
  "transition:background-color .3s,color .3s;padding-top:60px;padding-bottom:30px}" \
  "p,div,span,strong,label{color:var(--text-color)}" \
  ".navbar{position:fixed;top:0;left:0;right:0;background:var(--topnav-bg);" \
  "border-bottom:1px solid var(--border-color);box-shadow:0 2px 5px rgba(0,0,0,.3);" \
  "z-index:1000;display:flex;align-items:center;padding:0 16px;height:60px}" \
  ".nav-brand{font-size:1.1em;font-weight:bold;color:var(--topnav-text);margin-right:6px;white-space:nowrap}" \
  ".nav-sub{font-size:.75em;color:#aaa;font-family:monospace;margin-right:10px;white-space:nowrap}" \
  ".nav-tab{color:var(--topnav-text);text-decoration:none;padding:0 12px;font-size:.85em;" \
  "display:flex;align-items:center;height:60px;border-bottom:3px solid transparent;" \
  "box-sizing:border-box;white-space:nowrap;" \
  "background:none;border-left:none;border-right:none;border-top:none;cursor:pointer;font:inherit}" \
  ".nav-tab:hover{color:var(--topnav-hover-text);background:var(--topnav-hover)}" \
  ".nav-tab.active{border-bottom-color:#00bcd4;color:#00bcd4}" \
  ".nav-live{margin-left:auto;color:#00bcd4;font-family:monospace;font-size:.78em;" \
  "font-weight:bold;text-decoration:none;padding:5px 11px;border:1px solid #00bcd4;" \
  "border-radius:4px;letter-spacing:.08em;white-space:nowrap;background:none;cursor:pointer}" \
  ".nav-live:hover{background:#00bcd4;color:#000}" \
  ".theme-toggle{margin-left:8px;cursor:pointer;background:var(--topnav-hover);border:none;" \
  "padding:8px 12px;border-radius:50%;font-size:1.2em;color:var(--topnav-text);flex-shrink:0}" \
  ".theme-toggle:hover{background:#007bff;color:white}" \
  ".container{max-width:1100px;margin:20px auto;background:var(--container-bg);padding:20px;" \
  "border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,.1)}" \
  ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:15px;margin:10px 0}" \
  ".card{background:var(--card-bg);padding:15px;border-radius:6px;border:1px solid var(--border-color)}" \
  ".card h3{margin:0 0 12px 0;color:var(--text-color)}" \
  ".metric{display:flex;justify-content:space-between;align-items:center;padding:7px 0;" \
  "border-bottom:1px solid var(--border-color)}" \
  ".metric:last-child{border-bottom:none}" \
  ".metric-label{font-weight:bold;color:var(--text-muted);font-size:.88em}" \
  ".metric-value{color:var(--text-color);font-family:monospace;font-size:.9em;text-align:right}" \
  ".badge{display:inline-block;font-size:.75em;font-weight:bold;padding:2px 10px;border-radius:12px;color:#fff}" \
  ".badge-success{background:#28a745}" \
  ".badge-warning{background:#ffc107;color:#212529}" \
  ".badge-danger{background:#dc3545}" \
  ".clock-val{font-size:2.2em;font-family:monospace;letter-spacing:.08em;color:#00bcd4;margin:8px 0 4px}" \
  ".clock-sub{font-size:.8em;color:var(--text-muted)}" \
  ".pocsag-msg{font-family:monospace;font-size:1em;color:#ffb300;word-break:break-all;padding:6px 0;min-height:2em}" \
  ".switch{position:relative;display:inline-block;width:60px;height:34px;flex-shrink:0}" \
  ".switch input{opacity:0;width:0;height:0}" \
  ".slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background-color:#ccc;" \
  "transition:.4s;border-radius:34px;box-shadow:0 0 2px #000}" \
  ".slider:before{position:absolute;content:'';height:26px;width:26px;left:4px;bottom:4px;" \
  "background-color:white;transition:.4s;border-radius:50%}" \
  "input:checked+.slider{background-color:#4CAF50}" \
  "input:not(:checked)+.slider{background-color:#f44336}" \
  "input:checked+.slider:before{transform:translateX(26px)}" \
  ".bright-top{display:flex;align-items:center;gap:12px;margin-bottom:12px}" \
  ".bright-lbl{color:var(--text-muted);font-size:.88em;white-space:nowrap}" \
  ".bright-bot{display:flex;align-items:center;gap:10px}" \
  "input[type=range]{flex:1;accent-color:#00bcd4;cursor:pointer;min-width:0}" \
  "input[type=range]:disabled{opacity:.35;cursor:default}" \
  ".bright-num{color:var(--text-muted);font-size:.88em;min-width:30px;text-align:right;" \
  "font-family:monospace;flex-shrink:0}" \
  ".nav-more{position:relative}" \
  ".nav-more-menu{display:none;position:absolute;top:60px;left:0;background:var(--topnav-bg);" \
  "border:1px solid var(--border-color);min-width:130px;z-index:1001;" \
  "box-shadow:0 4px 8px rgba(0,0,0,.4)}" \
  ".nav-more-menu.open{display:block}" \
  ".nav-more-item{display:block;padding:11px 16px;color:var(--topnav-text);" \
  "text-decoration:none;font-size:.85em}" \
  ".nav-more-item:hover{background:var(--topnav-hover);color:var(--topnav-hover-text)}" \
  ".nav-more-item.active{color:#00bcd4}" \
  ".btn{display:inline-block;padding:7px 16px;border:none;border-radius:4px;cursor:pointer;" \
  "font-size:.88em;font-weight:bold;text-align:center;transition:background .2s;white-space:nowrap}" \
  ".btn:hover:not(:disabled){opacity:.88}" \
  ".btn:disabled{opacity:.5;cursor:not-allowed}" \
  ".btn-primary{background:#007bff;color:#fff}" \
  ".btn-success{background:#28a745;color:#fff}" \
  ".btn-danger{background:#dc3545;color:#fff}" \
  ".btn-warning{background:#e67e22;color:#fff}" \
  ".btn-secondary{background:#555;color:#fff}" \
  ".btn-info{background:#00bcd4;color:#000}" \
  ".modal-overlay{position:fixed;top:0;left:0;width:100%;height:100%;" \
  "background:rgba(0,0,0,.6);z-index:9999;display:flex;align-items:center;justify-content:center}" \
  ".modal-box{background:var(--card-bg);border:1px solid var(--border-color);border-radius:10px;" \
  "padding:24px;min-width:280px;max-width:90vw;color:var(--text-color);box-shadow:0 4px 20px rgba(0,0,0,.5)}" \
  ".modal-box h4{margin:0 0 14px 0;color:var(--text-color);word-break:break-all;line-height:1.4}" \
  ".modal-box input{width:100%;box-sizing:border-box;margin-bottom:10px;" \
  "background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);" \
  "border-radius:4px;padding:7px 9px;font-size:.92em}" \
  ".modal-buttons{display:flex;gap:8px;margin-top:16px}" \
  ".modal-buttons .btn{flex:1}"

// Inline theme init — placed in <head> to prevent flash of unstyled content.
// Also injects PWA/favicon meta tags so every page gets them automatically.
#define THEME_INIT_SCRIPT \
  "<meta name='theme-color' content='#1a1a1a'>" \
  "<meta name='apple-mobile-web-app-capable' content='yes'>" \
  "<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent'>" \
  "<meta name='apple-mobile-web-app-title' content='Ulanzi'>" \
  "<link rel='icon' type='image/svg+xml' href='/favicon.ico'>" \
  "<link rel='apple-touch-icon' href='/apple-touch-icon.png'>" \
  "<link rel='manifest' href='/manifest.json'>" \
  "<script>(function(){" \
  "var t=localStorage.getItem('theme')||'dark';" \
  "document.documentElement.setAttribute('data-theme',t);" \
  "})();</script>"

// Common JS — emit as "<script>" COMMON_JS "</script>" on every page
#define COMMON_JS \
  "function showModal(fn){" \
  "var ov=document.createElement('div');ov.className='modal-overlay';" \
  "var box=document.createElement('div');box.className='modal-box';" \
  "function close(){if(ov.parentNode)document.body.removeChild(ov);}" \
  "fn(box,close);ov.appendChild(box);" \
  "ov.addEventListener('click',function(e){if(e.target===ov)close();});" \
  "document.body.appendChild(ov);return ov;}" \
  "function showAlert(msg){showModal(function(box,close){" \
  "var h=document.createElement('h4');h.textContent=msg;box.appendChild(h);" \
  "var d=document.createElement('div');d.className='modal-buttons';" \
  "var ok=document.createElement('button');ok.textContent='OK';ok.className='btn btn-primary';" \
  "ok.onclick=close;d.appendChild(ok);box.appendChild(d);});}" \
  "function showConfirm(msg,onYes,onNo){showModal(function(box,close){" \
  "var h=document.createElement('h4');h.textContent=msg;box.appendChild(h);" \
  "var d=document.createElement('div');d.className='modal-buttons';" \
  "var ok=document.createElement('button');ok.textContent='Yes';ok.className='btn btn-danger';" \
  "ok.onclick=function(){close();if(onYes)onYes();};" \
  "var no=document.createElement('button');no.textContent='No';no.className='btn btn-secondary';" \
  "no.onclick=function(){close();if(onNo)onNo();};" \
  "d.appendChild(ok);d.appendChild(no);box.appendChild(d);" \
  "setTimeout(function(){no.focus();},50);});}" \
  "function toggleTheme(){" \
  "var t=document.documentElement.getAttribute('data-theme')==='dark'?'light':'dark';" \
  "document.documentElement.setAttribute('data-theme',t);" \
  "localStorage.setItem('theme',t);" \
  "document.getElementById('theme-btn').innerHTML=t==='dark'?'&#127769;':'&#9728;&#65039;';" \
  "}" \
  "document.addEventListener('DOMContentLoaded',function(){" \
  "var t=document.documentElement.getAttribute('data-theme');" \
  "document.getElementById('theme-btn').innerHTML=t==='dark'?'&#127769;':'&#9728;&#65039;';" \
  "});"
