#ifndef STYLES_H
#define STYLES_H

// CSS for the web interface — derived from the main project's styles.h.
// Supports light/dark mode via data-theme attribute on <html>.
static String getStyles() {
    String s;

    // ── CSS variables ──────────────────────────────────────────────────────
    s += ":root{"
         "--bg-color:#f0f0f0;--container-bg:white;--text-color:#333;--text-primary:#333;"
         "--text-muted:#6c757d;--border-color:#dee2e6;--card-bg:#f8f9fa;--bg-secondary:#f8f9fa;"
         "--info-bg:#e7f3ff;--topnav-bg:#333;--topnav-text:#f2f2f2;--topnav-hover:#ddd;"
         "--topnav-hover-text:black;--link-color:#007bff;}";

    s += "[data-theme='dark']{"
         "--bg-color:#1a1a1a;--container-bg:#2d2d2d;--text-color:#ffffff;--text-primary:#ffffff;"
         "--text-muted:#adb5bd;--border-color:#555;--card-bg:#3a3a3a;--bg-secondary:#3a3a3a;"
         "--info-bg:#1e3a5f;--topnav-bg:#000;--topnav-text:#f2f2f2;--topnav-hover:#444;"
         "--topnav-hover-text:#ffffff;--link-color:#4da6ff;}";

    // ── Base ──────────────────────────────────────────────────────────────
    s += "body{font-family:Arial,sans-serif;margin:0;padding-top:60px;"
         "background:var(--bg-color);color:var(--text-color);transition:background-color .3s,color .3s;}";
    s += "p,div,span,strong,label{color:var(--text-color);}";
    s += "pre{background:var(--card-bg);color:var(--text-color);padding:10px;border-radius:4px;"
         "overflow-x:auto;margin:0;font-size:12px;max-height:300px;overflow-y:auto;"
         "border:1px solid var(--border-color);font-family:monospace;white-space:pre-wrap;}";

    // ── Layout ────────────────────────────────────────────────────────────
    s += ".container{max-width:1000px;margin:20px auto;background:var(--container-bg);"
         "padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,.1);}";
    s += ".admin-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));"
         "gap:20px;margin:20px 0;}";

    // ── Navbar ────────────────────────────────────────────────────────────
    s += ".navbar{position:fixed;top:0;left:0;right:0;background:var(--topnav-bg);"
         "border-bottom:1px solid var(--border-color);box-shadow:0 2px 5px rgba(0,0,0,.3);"
         "z-index:1000;display:flex;align-items:center;padding:0 20px;height:60px;}";
    s += ".nav-brand{font-size:1.1em;font-weight:bold;color:var(--topnav-text);margin-right:20px;}";
    s += ".nav-sub{font-size:.8em;color:var(--topnav-text);opacity:.7;}";
    s += ".theme-toggle{margin-left:auto;cursor:pointer;background:var(--topnav-hover);"
         "border:none;padding:8px 12px;border-radius:50%;font-size:1.1em;color:var(--topnav-text);}";
    s += ".theme-toggle:hover{background:#007bff;color:white;}";

    // ── Typography ────────────────────────────────────────────────────────
    s += "h1{color:var(--text-color);border-bottom:2px solid #007bff;"
         "padding-bottom:10px;margin-top:0;text-align:center;}";
    s += "h3{color:var(--text-color);margin-top:0;}";

    // ── Forms ─────────────────────────────────────────────────────────────
    s += "input,select{background:var(--container-bg);color:var(--text-color);"
         "border:1px solid var(--border-color);padding:8px;border-radius:4px;}";
    s += "input:focus,select:focus{border-color:#007bff;outline:none;}";

    // ── Buttons ───────────────────────────────────────────────────────────
    s += "button,.btn{display:inline-block;padding:10px 20px;border:none;border-radius:4px;"
         "cursor:pointer;font-size:14px;font-weight:bold;transition:background-color .3s;"
         "text-decoration:none;text-align:center;box-sizing:border-box;margin:2px;}";
    s += "button:hover:not(:disabled),.btn:hover:not(:disabled){opacity:.9;}";
    s += ".btn-primary{background-color:#007bff;color:white;}";
    s += ".btn-primary:hover:not(:disabled){background-color:#0069d9;}";
    s += ".btn-success{background-color:#28a745;color:white;}";
    s += ".btn-success:hover:not(:disabled){background-color:#218838;}";
    s += ".btn-danger{background-color:#dc3545;color:white;}";
    s += ".btn-danger:hover:not(:disabled){background-color:#c82333;}";
    s += ".btn-warning{background-color:#ffc107;color:#212529;}";
    s += ".action-buttons-vertical{display:flex;flex-direction:column;gap:10px;margin-top:15px;}";
    s += ".action-buttons-vertical button,.action-buttons-vertical .btn{width:100%;margin:0;}";

    // ── Cards ─────────────────────────────────────────────────────────────
    s += ".card{background:var(--card-bg);padding:15px;border-radius:6px;"
         "border:1px solid var(--border-color);}";
    s += ".card h3{margin-top:0;}";
    s += ".info{padding:12px;background:var(--info-bg);border-left:4px solid #007bff;"
         "margin:10px 0;border-radius:0 4px 4px 0;}";

    // ── Metric rows ───────────────────────────────────────────────────────
    s += ".metric{display:flex;justify-content:space-between;align-items:center;"
         "padding:8px 0;border-bottom:1px solid var(--border-color);}";
    s += ".metric:last-child{border-bottom:none;}";
    s += ".metric-label{font-weight:bold;color:var(--text-muted);padding-right:8px;white-space:nowrap;}";
    s += ".metric input,.metric select{max-width:55%;box-sizing:border-box;}";
    s += ".metric input.full{max-width:none;width:100%;box-sizing:border-box;}";

    // ── Toggle switch (matches main project exactly) ───────────────────────
    s += ".switch{position:relative;display:inline-block;width:60px;height:34px;}";
    s += ".switch input{opacity:0;width:0;height:0;}";
    s += ".slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;"
         "background-color:#ccc;transition:.4s;border-radius:34px;box-shadow:0 0 2px #000;}";
    s += ".slider:before{position:absolute;content:'';height:26px;width:26px;"
         "left:4px;bottom:4px;background-color:white;transition:.4s;border-radius:50%;}";
    s += "input:checked+.slider{background-color:#4CAF50;}";
    s += "input:not(:checked)+.slider{background-color:#f44336;}";
    s += "input:checked+.slider:before{transform:translateX(26px);}";

    // ── Status badges ─────────────────────────────────────────────────────
    s += ".status-badge{padding:8px 16px;border-radius:6px;font-weight:bold;"
         "text-align:center;display:inline-block;}";
    s += ".badge-success{background:#28a745;color:white;}";
    s += ".badge-danger{background:#dc3545;color:white;}";
    s += ".badge-warning{background:#ffc107;color:black;}";
    s += ".badge-secondary{background:#6c757d;color:white;}";

    // ── Modal ─────────────────────────────────────────────────────────────
    s += ".modal-overlay{position:fixed;top:0;left:0;width:100%;height:100%;"
         "background:rgba(0,0,0,.6);z-index:9999;display:flex;align-items:center;justify-content:center;}";
    s += ".modal-box{background:var(--card-bg);border:1px solid var(--border-color);"
         "border-radius:10px;padding:24px;min-width:280px;max-width:90vw;color:var(--text-color);}";
    s += ".modal-box h4{margin:0 0 16px 0;color:var(--text-color);}";
    s += ".modal-buttons{display:flex;gap:8px;margin-top:16px;}";
    s += ".modal-buttons .btn{flex:1;}";

    // ── Footer ────────────────────────────────────────────────────────────
    s += ".footer-links{text-align:center;padding:0 20px 10px 20px;color:var(--text-color);font-size:12px;}";
    s += ".footer-links a{color:var(--link-color);text-decoration:none;}";
    s += ".footer-links a:hover{text-decoration:underline;}";
    s += ".copyright{text-align:center;padding:0 20px 20px 20px;color:var(--text-muted);font-size:11px;}";

    return s;
}

#endif // STYLES_H
