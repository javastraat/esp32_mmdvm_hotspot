#pragma once
// Shared navigation bar and inline LIVE modal for all three pages.
//
// Usage in each page body:
//   NAV_BAR          ← nav bar + More dropdown + active-tab JS
//   NAV_LIVE_MODAL   ← fullscreen LIVE overlay (place right after NAV_BAR)
// In the page <script> block also include: NAV_LIVE_JS

// ── Nav bar ──────────────────────────────────────────────────────────────
// Layout: [Brand] [Home] [More ▾ → Settings / System] ··· [► LIVE] [🌙]
// IP is kept in a hidden element so JS pages can still update it without errors.

#define NAV_BAR \
  "<div class=\"navbar\">" \
  "<span id=\"h1\" style=\"display:none\"></span>" \
  "<span id=\"sub\" style=\"display:none\"></span>" \
  "<a href=\"/\" class=\"nav-tab\" id=\"nt-home\">Home</a>" \
  "<div class=\"nav-more\" id=\"nav-more\">" \
    "<button class=\"nav-tab\" id=\"nt-more\" onclick=\"toggleMore(event)\">More &#9662;</button>" \
    "<div class=\"nav-more-menu\" id=\"nav-more-menu\">" \
      "<a href=\"/display\" class=\"nav-more-item\">Display</a>" \
      "<a href=\"/settings\" class=\"nav-more-item\">Settings</a>" \
      "<a href=\"/mqtt\" class=\"nav-more-item\">MQTT</a>" \
      "<a href=\"/espnow\" class=\"nav-more-item\">ESP-NOW</a>" \
      "<a href=\"/files\" class=\"nav-more-item\">Files</a>" \
      "<a href=\"/info\" class=\"nav-more-item\">Info</a>" \
      "<a href=\"/serial\" class=\"nav-more-item\">Serial</a>" \
    "</div>" \
  "</div>" \
  "<button class=\"nav-live\" onclick=\"showLive()\">&#9654; LIVE</button>" \
  "<button class=\"theme-toggle\" id=\"theme-btn\" onclick=\"toggleTheme()\">&#127769;</button>" \
  "</div>" \
  "<script>" \
  "function toggleMore(e){" \
    "e.stopPropagation();" \
    "document.getElementById('nav-more-menu').classList.toggle('open');}" \
  "(function(){" \
    "var p=location.pathname;" \
    "var id=p==='/'?'nt-home':'nt-more';" \
    "var el=document.getElementById(id);if(el)el.classList.add('active');" \
    "if(p!=='/'){var items=document.querySelectorAll('.nav-more-item');" \
      "for(var i=0;i<items.length;i++)" \
        "if(items[i].getAttribute('href')===p)items[i].classList.add('active');}" \
    "document.addEventListener('click',function(){" \
      "document.getElementById('nav-more-menu').classList.remove('open');});" \
  "})();" \
  "</script>"

// ── Fullscreen LIVE modal overlay ─────────────────────────────────────────
// Canvas spans full viewport width; aspect-ratio:4/1 maintains 32×8 proportions.
// Close via ✕ button or Esc key.

#define NAV_LIVE_MODAL \
  "<div id=\"live-modal\" style=\"" \
    "display:none;position:fixed;top:0;left:0;right:0;bottom:0;" \
    "background:#0a0a0a;z-index:2000;flex-direction:column;" \
    "align-items:stretch;justify-content:center\">" \
  "<div style=\"display:flex;justify-content:space-between;align-items:center;" \
    "padding:0 16px;height:40px;flex-shrink:0\">" \
  "<span style=\"color:#555;font-family:monospace;font-size:.7em;" \
    "letter-spacing:.15em;text-transform:uppercase\">&#9679; Live Display</span>" \
  "<button onclick=\"hideLive()\" style=\"background:none;border:none;color:#666;" \
    "font-size:1.5em;cursor:pointer;line-height:1;padding:4px 8px\">&#x2715;</button>" \
  "</div>" \
  "<canvas id=\"live-c\" width=\"640\" height=\"160\" style=\"" \
    "display:block;width:100%;aspect-ratio:4/1;" \
    "background:#111;image-rendering:pixelated\"></canvas>" \
  "</div>"

// ── Live modal JS ─────────────────────────────────────────────────────────
// Include in each page's <script> block alongside COMMON_JS.

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
