#pragma once
// Shared navigation bar and inline LIVE modal for all three pages.
//
// Usage in each page:
//   NAV_BAR          ← nav bar HTML (just after <body>)
//   NAV_LIVE_MODAL   ← modal overlay HTML (just after NAV_BAR)
//   In the <script> block: NAV_LIVE_JS

// ── Nav bar ──────────────────────────────────────────────────────────────
// Active tab highlighted client-side via window.location.pathname.
// LIVE button opens the inline modal instead of a new tab.

#define NAV_BAR \
  "<div class=\"navbar\">" \
  "<span class=\"nav-brand\" id=\"h1\">Ulanzi</span>" \
  "<span class=\"nav-sub\" id=\"sub\"></span>" \
  "<a href=\"/\" class=\"nav-tab\" id=\"nt-home\">Home</a>" \
  "<a href=\"/settings\" class=\"nav-tab\" id=\"nt-set\">Settings</a>" \
  "<a href=\"/system\" class=\"nav-tab\" id=\"nt-sys\">System</a>" \
  "<button class=\"nav-live\" onclick=\"showLive()\">&#9654; LIVE</button>" \
  "<button class=\"theme-toggle\" id=\"theme-btn\" onclick=\"toggleTheme()\">&#127769;</button>" \
  "</div>" \
  "<script>(function(){" \
  "var p=location.pathname;" \
  "var id=p==='/'?'nt-home':p.startsWith('/settings')?'nt-set':'nt-sys';" \
  "var el=document.getElementById(id);if(el)el.classList.add('active');" \
  "})();</script>"

// ── Inline LIVE modal overlay ─────────────────────────────────────────────
// Place immediately after NAV_BAR in the page body.

#define NAV_LIVE_MODAL \
  "<div id=\"live-modal\" onclick=\"hideLive()\" style=\"" \
    "display:none;position:fixed;top:0;left:0;right:0;bottom:0;" \
    "background:rgba(0,0,0,.92);z-index:2000;" \
    "flex-direction:column;align-items:center;justify-content:center;gap:14px\">" \
  "<p style=\"color:#444;font-family:monospace;font-size:.72em;letter-spacing:.2em;" \
    "text-transform:uppercase\">&#9679; Display Live</p>" \
  "<div style=\"width:min(640px,95vw)\" onclick=\"event.stopPropagation()\">" \
  "<canvas id=\"live-c\" width=\"640\" height=\"160\" style=\"" \
    "display:block;width:100%;background:#111;border-radius:6px;image-rendering:pixelated\">" \
  "</canvas></div>" \
  "<p style=\"color:#555;font-family:monospace;font-size:.7em\">" \
    "click outside or press Esc to close</p>" \
  "</div>"

// ── Live modal JS ─────────────────────────────────────────────────────────
// Included in each page's <script> block alongside COMMON_JS.

#define NAV_LIVE_JS \
  "var _lt=null;" \
  "function showLive(){" \
    "var m=document.getElementById('live-modal');m.style.display='flex';_lp();}" \
  "function hideLive(){" \
    "document.getElementById('live-modal').style.display='none';" \
    "if(_lt){clearTimeout(_lt);_lt=null;}}" \
  "function _lp(){" \
    "if(document.getElementById('live-modal').style.display==='none')return;" \
    "fetch('/api/leds').then(function(r){return r.text();}).then(function(hex){" \
      "var c=document.getElementById('live-c'),ctx=c.getContext('2d'),W=32,H=8,S=20;" \
      "ctx.fillStyle='#111';ctx.fillRect(0,0,c.width,c.height);" \
      "for(var y=0;y<H;y++)for(var x=0;x<W;x++){" \
        "var idx=(y%2===0)?y*W+x:(y+1)*W-1-x;" \
        "var r=parseInt(hex.substr(idx*6,2),16);" \
        "var g=parseInt(hex.substr(idx*6+2,2),16);" \
        "var b=parseInt(hex.substr(idx*6+4,2),16);" \
        "if(r>0||g>0||b>0){ctx.fillStyle='rgb('+r+','+g+','+b+')';ctx.fillRect(x*S+1,y*S+1,S-2,S-2);}}" \
    "}).catch(function(){});" \
    "_lt=setTimeout(_lp,250);}" \
  "document.addEventListener('keydown',function(e){if(e.key==='Escape')hideLive();});"
