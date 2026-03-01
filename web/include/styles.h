/*
 * styles.h - Common CSS and Theme for ESP32 RTOS MMDVM Web Interface
 *
 * CSS Organization (ITCSS / outside-in):
 *  1. CSS Variables / Theme
 *  2. Base / Reset
 *  3. Layout (container, grid)
 *  4. Navigation (navbar, dropdowns, topnav)
 *  5. Typography (headings, text)
 *  6. Forms & Inputs
 *  7. Buttons
 *  8. Tables
 *  9. Cards & Panels
 * 10. Components (status, metrics, progress, toggles, etc.)
 * 11. Footer
 * 12. Utilities
 * 13. Media Queries (responsive)
 */

#ifndef WEB_STYLES_H
#define WEB_STYLES_H

#include <Arduino.h>

String getSharedStyles() {
  extern String userCallsign;
  // Inline SVG favicon (antenna icon) - works for bookmarks, no extra HTTP request
  //
  //black icon
  //static const char FAVICON_SVG[] PROGMEM = R"rawsvg(<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 40 40'><g><path fill='#010101' d='M22.603,19.604c1.136-0.807,1.88-2.131,1.88-3.626c0-2.452-1.994-4.446-4.444-4.446c-2.451,0-4.445,1.994-4.445,4.446c0,1.495,0.745,2.82,1.881,3.627l-5.008,14.432c-0.132,0.378,0.068,0.791,0.447,0.924C12.991,34.986,13.072,35,13.15,35c0.3,0,0.581-0.188,0.684-0.488l1.02-2.933h10.37l1.017,2.933C26.344,34.813,26.626,35,26.927,35c0.078,0,0.157-0.014,0.237-0.039c0.378-0.133,0.578-0.546,0.446-0.924L22.603,19.604z M20.038,12.991c1.646,0,2.986,1.34,2.986,2.987c0,1.646-1.34,2.986-2.986,2.986c-1.647,0-2.986-1.34-2.986-2.986C17.052,14.331,18.391,12.991,20.038,12.991z M20.038,20.423c0.435,0,0.854-0.064,1.252-0.181l1.603,4.622h-5.709l1.604-4.622C19.185,20.359,19.604,20.423,20.038,20.423z M15.356,30.129l1.324-3.814h6.715l1.325,3.814H15.356z'/><path fill='#010101' d='M14.12,21.617c0.142,0.142,0.327,0.212,0.511,0.212c0.187,0,0.373-0.07,0.515-0.212c0.282-0.283,0.282-0.743,0-1.025c-1.234-1.231-1.913-2.87-1.913-4.613c0-1.743,0.678-3.381,1.911-4.613c0.282-0.283,0.282-0.742,0-1.026c-0.285-0.282-0.743-0.282-1.025,0c-1.507,1.506-2.336,3.508-2.336,5.639C11.782,18.108,12.612,20.111,14.12,21.617z'/><path fill='#010101' d='M12.581,23.154c-1.919-1.916-2.974-4.465-2.974-7.176c0-2.71,1.055-5.259,2.973-7.178c0.283-0.283,0.283-0.742,0-1.025c-0.284-0.283-0.743-0.282-1.025,0.001c-2.191,2.191-3.398,5.104-3.398,8.202s1.208,6.011,3.4,8.202c0.142,0.142,0.326,0.212,0.512,0.212s0.372-0.07,0.513-0.212C12.864,23.896,12.864,23.438,12.581,23.154z'/><path fill='#010101' d='M5.982,15.979c0-3.678,1.432-7.137,4.034-9.741c0.283-0.283,0.283-0.742,0-1.025c-0.283-0.283-0.743-0.283-1.025,0c-2.876,2.877-4.46,6.7-4.46,10.766c0,4.065,1.584,7.889,4.46,10.766c0.142,0.141,0.327,0.211,0.513,0.211s0.372-0.07,0.513-0.211c0.283-0.284,0.283-0.744,0-1.026C7.415,23.115,5.982,19.656,5.982,15.979z'/><path fill='#010101' d='M26.768,15.979c0,1.742-0.679,3.38-1.911,4.612c-0.282,0.282-0.282,0.741,0,1.024c0.142,0.142,0.328,0.213,0.514,0.213c0.185,0,0.37-0.071,0.512-0.213c1.507-1.506,2.337-3.508,2.337-5.637c0-2.131-0.831-4.134-2.338-5.64c-0.283-0.283-0.741-0.282-1.026,0c-0.282,0.283-0.282,0.742,0.001,1.025C26.089,12.596,26.768,14.235,26.768,15.979z'/><path fill='#010101' d='M28.445,7.775c-0.285-0.283-0.744-0.282-1.026,0.001c-0.284,0.283-0.284,0.742,0,1.024c1.917,1.917,2.973,4.466,2.973,7.178c0,2.709-1.055,5.258-2.971,7.178c-0.284,0.282-0.284,0.74,0,1.024c0.142,0.142,0.328,0.212,0.513,0.212c0.186,0,0.372-0.07,0.513-0.212c2.19-2.192,3.397-5.105,3.397-8.202C31.844,12.879,30.636,9.966,28.445,7.775z'/><path fill='#010101' d='M31.007,5.212c-0.282-0.282-0.741-0.282-1.025,0c-0.283,0.283-0.283,0.743,0,1.025c2.603,2.603,4.037,6.063,4.037,9.741c0,3.677-1.434,7.136-4.035,9.74c-0.283,0.283-0.283,0.741,0,1.025c0.141,0.141,0.327,0.211,0.513,0.211c0.185,0,0.371-0.07,0.513-0.211c2.876-2.879,4.459-6.701,4.459-10.766C35.468,11.912,33.884,8.089,31.007,5.212z'/></g></svg>)rawsvg";
  // iOS / PWA meta tags — must appear in <head>
  String css = "<meta name='apple-mobile-web-app-capable' content='yes'>";
  css += "<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent'>";
  css += "<meta name='apple-mobile-web-app-title' content='" + userCallsign + "'>";
  css += "<meta name='theme-color' content='#333333'>";
  css += "<link rel='apple-touch-icon' href='/apple-touch-icon.png'>";
  css += "<link rel='manifest' href='/manifest.json'>";
  css += "<link rel='icon' type='image/svg+xml' href=\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 40 40'%3E%3Crect width='40' height='40' fill='black'/%3E%3Cg%3E%3Cpath fill='white' d='M22.603,19.604c1.136-0.807,1.88-2.131,1.88-3.626c0-2.452-1.994-4.446-4.444-4.446c-2.451,0-4.445,1.994-4.445,4.446c0,1.495,0.745,2.82,1.881,3.627l-5.008,14.432c-0.132,0.378,0.068,0.791,0.447,0.924C12.991,34.986,13.072,35,13.15,35c0.3,0,0.581-0.188,0.684-0.488l1.02-2.933h10.37l1.017,2.933C26.344,34.813,26.626,35,26.927,35c0.078,0,0.157-0.014,0.237-0.039c0.378-0.133,0.578-0.546,0.446-0.924L22.603,19.604z M20.038,12.991c1.646,0,2.986,1.34,2.986,2.987c0,1.646-1.34,2.986-2.986,2.986c-1.647,0-2.986-1.34-2.986-2.986C17.052,14.331,18.391,12.991,20.038,12.991z M20.038,20.423c0.435,0,0.854-0.064,1.252-0.181l1.603,4.622h-5.709l1.604-4.622C19.185,20.359,19.604,20.423,20.038,20.423z M15.356,30.129l1.324-3.814h6.715l1.325,3.814H15.356z'/%3E%3Cpath fill='white' d='M14.12,21.617c0.142,0.142,0.327,0.212,0.511,0.212c0.187,0,0.373-0.07,0.515-0.212c0.282-0.283,0.282-0.743,0-1.025c-1.234-1.231-1.913-2.87-1.913-4.613c0-1.743,0.678-3.381,1.911-4.613c0.282-0.283,0.282-0.742,0-1.026c-0.285-0.282-0.743-0.282-1.025,0c-1.507,1.506-2.336,3.508-2.336,5.639C11.782,18.108,12.612,20.111,14.12,21.617z'/%3E%3Cpath fill='white' d='M12.581,23.154c-1.919-1.916-2.974-4.465-2.974-7.176c0-2.71,1.055-5.259,2.973-7.178c0.283-0.283,0.283-0.742,0-1.025c-0.284-0.283-0.743-0.282-1.025,0.001c-2.191,2.191-3.398,5.104-3.398,8.202s1.208,6.011,3.4,8.202c0.142,0.142,0.326,0.212,0.512,0.212s0.372-0.07,0.513-0.212C12.864,23.896,12.864,23.438,12.581,23.154z'/%3E%3Cpath fill='white' d='M5.982,15.979c0-3.678,1.432-7.137,4.034-9.741c0.283-0.283,0.283-0.742,0-1.025c-0.283-0.283-0.743-0.283-1.025,0c-2.876,2.877-4.46,6.7-4.46,10.766c0,4.065,1.584,7.889,4.46,10.766c0.142,0.141,0.327,0.211,0.513,0.211s0.372-0.07,0.513-0.211c0.283-0.284,0.283-0.744,0-1.026C7.415,23.115,5.982,19.656,5.982,15.979z'/%3E%3Cpath fill='white' d='M26.768,15.979c0,1.742-0.679,3.38-1.911,4.612c-0.282,0.282-0.282,0.741,0,1.024c0.142,0.142,0.328,0.213,0.514,0.213c0.185,0,0.37-0.071,0.512-0.213c1.507-1.506,2.337-3.508,2.337-5.637c0-2.131-0.831-4.134-2.338-5.64c-0.283-0.283-0.741-0.282-1.026,0c-0.282,0.283-0.282,0.742,0.001,1.025C26.089,12.596,26.768,14.235,26.768,15.979z'/%3E%3Cpath fill='white' d='M28.445,7.775c-0.285-0.283-0.744-0.282-1.026,0.001c-0.284,0.283-0.284,0.742,0,1.024c1.917,1.917,2.973,4.466,2.973,7.178c0,2.709-1.055,5.258-2.971,7.178c-0.284,0.282-0.284,0.74,0,1.024c0.142,0.142,0.328,0.212,0.513,0.212c0.186,0,0.372-0.07,0.513-0.212c2.19-2.192,3.397-5.105,3.397-8.202C31.844,12.879,30.636,9.966,28.445,7.775z'/%3E%3Cpath fill='white' d='M31.007,5.212c-0.282-0.282-0.741-0.282-1.025,0c-0.283,0.283-0.283,0.743,0,1.025c2.603,2.603,4.037,6.063,4.037,9.741c0,3.677-1.434,7.136-4.035,9.74c-0.283,0.283-0.283,0.741,0,1.025c0.141,0.141,0.327,0.211,0.513,0.211c0.185,0,0.371-0.07,0.513-0.211c2.876-2.879,4.459-6.701,4.459-10.766C35.468,11.912,33.884,8.089,31.007,5.212z'/%3E%3C/g%3E%3C/svg%3E\">";
  
  //blue icon
  //String css = "<link rel='icon' type='image/svg+xml' href=\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'%3E%3Crect width='64' height='64' rx='12' fill='%23222'/%3E%3Cpath d='M32 18v28' stroke='%2300bfff' stroke-width='5' stroke-linecap='round'/%3E%3Cpath d='M22 28a14 14 0 0 1 20 0' stroke='%2300bfff' stroke-width='4' fill='none' stroke-linecap='round'/%3E%3Cpath d='M16 22a22 22 0 0 1 32 0' stroke='%2300bfff' stroke-width='3.5' fill='none' stroke-linecap='round'/%3E%3Ccircle cx='32' cy='50' r='5' fill='%2300bfff'/%3E%3C/svg%3E\">";
  
  //String css = "<link rel='icon' type='image/svg+xml' href=\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'%3E%3Crect width='64' height='64' rx='12' fill='%23222'/%3E%3Cpath d='M32 18v28' stroke='%2300bfff' stroke-width='5' stroke-linecap='round'/%3E%3Cpath d='M22 28a14 14 0 0 1 20 0' stroke='%2300bfff' stroke-width='4' fill='none' stroke-linecap='round'/%3E%3Cpath d='M16 22a22 22 0 0 1 32 0' stroke='%2300bfff' stroke-width='3.5' fill='none' stroke-linecap='round'/%3E%3Ccircle cx='32' cy='50' r='5' fill='%2300bfff'/%3E%3C/svg%3E\">";
  css += "<style>";

  // ===== 1. CSS Variables / Theme =====
  css += ":root { --bg-color: #f0f0f0; --container-bg: white; --text-color: #333; --text-primary: #333; --text-muted: #6c757d; --border-color: #dee2e6; --card-bg: #f8f9fa; --bg-secondary: #f8f9fa; --info-bg: #e7f3ff; --topnav-bg: #333; --topnav-text: #f2f2f2; --topnav-hover: #ddd; --topnav-hover-text: black; --link-color: #007bff; --link-hover-color: #0056b3; }";
  css += "[data-theme='dark'] { --bg-color: #1a1a1a; --container-bg: #2d2d2d; --text-color: #ffffff; --text-primary: #ffffff; --text-muted: #adb5bd; --border-color: #555; --card-bg: #3a3a3a; --bg-secondary: #3a3a3a; --info-bg: #1e3a5f; --topnav-bg: #000; --topnav-text: #f2f2f2; --topnav-hover: #444; --topnav-hover-text: #ffffff; --link-color: #4da6ff; --link-hover-color: #66b3ff; }";

  // ===== 2. Base / Reset =====
  css += "body { font-family: Arial, sans-serif; margin: 0; padding-top: 50px; background: var(--bg-color); color: var(--text-color); transition: background-color 0.3s, color 0.3s; }";
  css += "p, div, span, strong, label { color: var(--text-color); }";
  css += "code { background: var(--bg-secondary); padding: 2px 5px; border-radius: 3px; font-family: monospace; }";
  css += "pre { background: var(--card-bg); color: var(--text-color); padding: 10px; border-radius: 4px; overflow-x: auto; margin: 0; font-size: 12px; max-height: 300px; overflow-y: auto; border: 1px solid var(--border-color); font-family: monospace; white-space: pre-wrap; }";

  // ===== 3. Layout (container, grid) =====
  css += ".container { max-width: 1000px; margin: 20px auto; background: var(--container-bg); padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
  css += ".grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 15px; margin: 20px 0; }";
  css += ".admin-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 20px 0; }";

  // ===== 4. Navigation =====

  // Navbar (primary navigation)
  css += ".navbar{position:fixed;top:0;left:0;right:0;background:var(--topnav-bg);border-bottom:1px solid var(--border-color);box-shadow:0 2px 5px rgba(0,0,0,0.3);z-index:1000;display:flex;align-items:center;padding:0 20px;height:60px}";
  css += ".nav-brand{font-size:1.2em;font-weight:bold;color:var(--topnav-text);margin-right:30px}";
  css += ".nav-menu{display:flex;align-items:center;flex:1}";
  css += ".nav-item{margin-right:20px;position:relative}";
  css += ".nav-link{color:var(--topnav-text);padding:10px 15px;display:block;border-radius:5px;background:var(--topnav-bg);text-decoration:none}";
  css += ".nav-link:hover{background:var(--topnav-hover);text-decoration:none;color:var(--topnav-hover-text)}";
  css += ".nav-link.active{background:var(--topnav-hover);color:var(--topnav-hover-text);text-decoration:none}";

  // Dropdowns
  css += ".dropdown{position:relative}";
  css += ".dropdown-toggle{cursor:pointer;user-select:none;background:var(--topnav-bg);text-decoration:none;border-radius:5px;padding:10px 15px;transition:background 0.2s;color:var(--topnav-text)}";
  css += ".dropdown-toggle:hover,.dropdown.active .dropdown-toggle{background:var(--topnav-hover);color:var(--topnav-hover-text);text-decoration:none}";
  css += ".dropdown-toggle::after{content:'▼';font-size:0.7em;margin-left:5px}";
  css += ".dropdown-menu{display:none;position:absolute;top:100%;left:0;background:var(--topnav-bg)!important;z-index:1001;border:1px solid var(--border-color);border-radius:5px;box-shadow:0 4px 10px rgba(0,0,0,0.4);min-width:200px;margin-top:5px}";
  css += ".dropdown-menu a{padding:10px 15px;display:block;color:var(--topnav-text)!important;background:var(--topnav-bg)!important;border-bottom:1px solid var(--border-color);text-decoration:none}";
  css += ".dropdown-menu a:last-child{border-bottom:none}";
  css += ".dropdown-menu a:hover,.dropdown-menu a.active{background:var(--topnav-hover)!important;color:var(--topnav-hover-text)!important;text-decoration:none}";
  css += ".dropdown.active .dropdown-menu{display:block}";

  // Dropdown submenus
  css += ".dropdown-submenu{position:relative}";
  css += ".dropdown-submenu>span{padding:10px 15px;display:block;color:var(--topnav-text)!important;background:var(--topnav-bg)!important;border-bottom:1px solid var(--border-color);cursor:pointer}";
  css += ".dropdown-submenu>span::after{content:'▶';float:right;font-size:0.8em}";
  css += ".dropdown-submenu>span:hover{background:var(--topnav-hover)!important;color:var(--topnav-hover-text)!important}";
  css += ".dropdown-submenu-menu{display:none;position:absolute;left:100%;top:0;background:var(--topnav-bg)!important;z-index:1002;border:1px solid var(--border-color);border-radius:5px;box-shadow:0 4px 10px rgba(0,0,0,0.4);min-width:180px}";
  css += ".dropdown-submenu-menu a{padding:8px 15px;display:block;color:var(--topnav-text)!important;background:var(--topnav-bg)!important;border-bottom:1px solid var(--border-color);text-decoration:none}";
  css += ".dropdown-submenu-menu a:last-child{border-bottom:none}";
  css += ".dropdown-submenu-menu a:hover,.dropdown-submenu-menu a.active{background:var(--topnav-hover)!important;color:var(--topnav-hover-text)!important;text-decoration:none}";
  css += ".dropdown-submenu.active .dropdown-submenu-menu{display:block}";

  // Topnav (legacy/simple navigation)
  css += ".topnav { position: fixed; top: 0; width: 100%; z-index: 1000; background-color: var(--topnav-bg); overflow: hidden; }";
  css += ".topnav a { float: left; display: block; color: var(--topnav-text); text-align: center; padding: 14px 20px; text-decoration: none; background-color: var(--topnav-bg); }";
  css += ".topnav a:hover { background-color: var(--topnav-hover); color: var(--topnav-hover-text); }";
  css += ".topnav a.active { background-color: #007bff; color: white; }";
  css += ".topnav .icon { display: none; }";

  // Theme toggle (navbar variant)
  css += ".theme-toggle{margin-left:auto;cursor:pointer;background:var(--topnav-hover);border:none;padding:10px 15px;border-radius:50%;font-size:1.2em;color:var(--topnav-text)}";
  css += ".theme-toggle:hover{background:#007bff;color:white}";

  // ===== 5. Typography =====
  css += "h1 { color: var(--text-color); border-bottom: 2px solid #007bff; padding-bottom: 10px; margin-top: 0; }";
  css += "h2 { color: var(--text-color); margin-top: 30px; }";
  css += "h3 { color: var(--text-color); }";

  // ===== 6. Forms & Inputs =====
  css += "input, select, textarea { background: var(--container-bg); color: var(--text-color); border: 1px solid var(--border-color); padding: 8px; border-radius: 4px; }";
  css += "input:focus, select:focus, textarea:focus { border-color: #007bff; outline: none; }";
  css += "input[type=text] { padding: 8px; border-radius: 4px; }";
  css += ".search-input { width: 100%; max-width: 300px; padding: 8px; border: 1px solid var(--border-color); border-radius: 4px; font-size: 14px; box-sizing: border-box; }";

  // ===== 7. Buttons =====
  css += "button, .btn { display: inline-block; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; font-size: 14px; font-weight: bold; transition: background-color 0.3s ease; text-decoration: none; text-align: center; box-sizing: border-box; margin: 2px; }";
  css += "button:hover:not(:disabled), .btn:hover:not(:disabled) { opacity: 0.9; }";
  css += "button:disabled, .btn:disabled { opacity: 0.6; cursor: not-allowed; background-color: #cccccc !important; }";
  css += ".btn-primary { background-color: #007bff; color: white; }";
  css += ".btn-primary:hover:not(:disabled) { background-color: #0069d9; }";
  css += ".btn-success { background-color: #28a745; color: white; }";
  css += ".btn-success:hover:not(:disabled) { background-color: #218838; }";
  css += ".btn-danger { background-color: #dc3545; color: white; }";
  css += ".btn-danger:hover:not(:disabled) { background-color: #c82333; }";
  css += ".btn-danger:disabled { background-color: #dc3545 !important; opacity: 0.6; }";
  css += ".btn-warning { background-color: #ffc107; color: #212529; }";
  css += ".btn-warning:hover:not(:disabled) { background-color: #e0a800; }";
  css += ".btn-secondary { background-color: #6c757d; color: white; }";
  css += ".btn-secondary:hover:not(:disabled) { background-color: #545b62; }";
  css += ".btn-enable-muted { background-color: #6b8e7f !important; color: white; border-color: #6b8e7f; }";
  css += ".btn-enable-muted:hover:not(:disabled) { background-color: #5a7d6e !important; }";
  css += ".btn-enable-muted:disabled { background-color: #6b8e7f !important; opacity: 0.6; }";
  css += ".btn-disable-muted { background-color: #a85959 !important; color: white; border-color: #a85959; }";
  css += ".btn-disable-muted:hover:not(:disabled) { background-color: #944a4a !important; }";
  css += ".btn-disable-muted:disabled { background-color: #a85959 !important; opacity: 0.6; }";
  css += ".action-buttons-vertical { display: flex; flex-direction: column; gap: 10px; margin-top: 15px; }";
  css += ".action-buttons-vertical button, .action-buttons-vertical .btn { width: 100%; margin: 0; }";

  // ===== 8. Tables =====
  css += "table { width: 100%; border-collapse: collapse; margin-top: 10px; }";
  css += "table th, table td { padding: 8px; text-align: left; border: 1px solid var(--border-color); color: var(--text-color); }";
  css += "table th { background: var(--card-bg); font-weight: bold; }";
  css += "tr:nth-child(even) { background: var(--card-bg); }";

  // ===== 9. Cards & Panels =====
  css += ".card { background: var(--card-bg); padding: 15px; border-radius: 6px; border: 1px solid var(--border-color); }";
  css += ".card h3 { margin-top: 0; color: var(--text-color); }";
  css += ".info { padding: 12px; background: var(--info-bg); border-left: 4px solid #007bff; margin: 10px 0; border-radius: 0 4px 4px 0; }";

  // ===== 10. Components =====

  // Status indicators
  css += ".status { padding: 12px; margin: 10px 0; border-radius: 6px; font-weight: bold; }";
  css += ".status.connected { background: #d4edda; border: 1px solid #c3e6cb; color: #155724; }";
  css += ".status.disconnected { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; }";
  css += ".status.warning { background: #fff3cd; border: 1px solid #ffeaa7; color: #856404; }";
  css += ".status-text { padding: 10px; background: var(--bg-secondary); border-radius: 4px; margin: 10px 0; color: var(--text-color); }";
  css += ".update-available { color: #ff9800; font-weight: bold; margin: 10px 0; }";
  css += ".up-to-date { color: #4CAF50; font-weight: bold; margin: 10px 0; }";
  css += ".status-badge-wrap { margin-top: 10px; display: flex; justify-content: center; }";
  css += ".status-badge { padding: 8px 16px; border-radius: 6px; font-weight: bold; text-align: center; display: inline-block; }";
  css += ".status-badge.badge-success { background: #28a745; color: white; }";
  css += ".status-badge.badge-danger { background: #dc3545; color: white; }";
  css += ".status-badge.badge-warning { background: #ffc107; color: black; }";

  // Metric display
  css += ".metric { display: flex; justify-content: space-between; align-items: center; padding: 8px 0; border-bottom: 1px solid var(--border-color); }";
  css += ".metric:last-child { border-bottom: none; }";
  css += ".metric .metric-label { font-weight: bold; color: var(--text-muted); }";
  css += ".metric .metric-value { font-weight: normal; color: var(--text-color); }";
  css += ".metric-label { color: var(--text-color) !important; padding-right: 8px; }";
  css += ".metric-value { color: var(--text-color) !important; }";

  // Progress bar
  css += ".progress-container { margin: 15px 0; display: none; }";
  css += ".progress-bar { width: 100%; height: 30px; background: var(--border-color); border-radius: 4px; overflow: hidden; position: relative; }";
  css += ".progress-fill { height: 100%; background: linear-gradient(90deg, #28a745, #34ce57); width: 0%; transition: width 0.3s; position: absolute; top: 0; left: 0; }";
  css += ".progress-text { position: absolute; width: 100%; text-align: center; line-height: 30px; color: var(--text-color); font-weight: bold; z-index: 1; top: 0; }";
  css += ".progress-bar-fill { height: 100%; background: linear-gradient(90deg, #4CAF50, #45a049); transition: width 0.3s; display: flex; align-items: center; justify-content: center; color: white; font-weight: bold; font-size: 14px; }";

  // Toggle switch
  css += ".switch { position: relative; display: inline-block; width: 60px; height: 34px; }";
  css += ".switch input { opacity: 0; width: 0; height: 0; }";
  css += ".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 34px; box-shadow: 0 0 2px #000; }";
  css += ".slider:before { position: absolute; content: ''; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }";
  css += "input:checked + .slider { background-color: #4CAF50; }";
  css += "input:not(:checked) + .slider { background-color: #f44336; }";
  css += "input:checked + .slider:before { transform: translateX(26px); }";
  css += ".switch input:focus + .slider { box-shadow: 0 0 1px #2196F3; }";

  // File list and code display
  css += ".file-list { color: var(--text-color); font-family: monospace; font-size: 12px; white-space: pre-wrap; word-break: break-all; }";
  css += ".owner-text { color: var(--text-color); padding: 10px 0; line-height: 1.5; white-space: pre-wrap; }";

  // Modal / Popup overlay
  css += ".modal-overlay { position:fixed; top:0; left:0; width:100%; height:100%; background:rgba(0,0,0,0.6); z-index:9999; display:flex; align-items:center; justify-content:center; }";
  css += ".modal-box { background:var(--card-bg); border:1px solid var(--border-color); border-radius:10px; padding:24px; min-width:280px; max-width:90vw; color:var(--text-color); }";
  css += ".modal-box h4 { margin:0 0 16px 0; color:var(--text-color); }";
  css += ".modal-box input, .modal-box select { width:100%; box-sizing:border-box; margin-bottom:12px; }";
  css += ".modal-buttons { display:flex; gap:8px; margin-top:16px; }";
  css += ".modal-buttons .btn { flex:1; }";

  // ===== 11. Footer =====
  css += ".footer { text-align: center; padding: 20px 20px 5px 20px; margin-top: 30px; border-top: 1px solid var(--border-color); color: var(--text-color); font-size: 14px; }";
  css += ".footer-links { text-align: center; padding: 0px 20px 10px 20px; color: var(--text-color); font-size: 12px; }";
  css += ".footer-links a { color: var(--link-color); text-decoration: none; }";
  css += ".footer-links a:hover { color: var(--link-hover-color); text-decoration: underline; }";
  css += ".copyright { text-align: center; padding: 0px 20px 20px 20px; color: var(--text-muted); font-size: 11px; }";

  // ===== 12. Media Queries (responsive) =====
  css += "@media screen and (max-width: 600px) {";
  css += "  .topnav a:not(:first-child) {display: none;}";
  css += "  .topnav a.icon {float: right; display: block;}";
  css += "  .topnav.responsive {position: relative;}";
  css += "  .topnav.responsive .icon {position: absolute; right: 0; top: 0;}";
  css += "  .topnav.responsive a {float: none; display: block; text-align: left;}";
  // Dropdown submenu: open below instead of to the right
  css += "  .dropdown-submenu-menu{position:static!important;left:auto!important;box-shadow:none!important;border:none!important;border-left:3px solid #007bff!important;border-radius:0!important;margin-left:10px;min-width:auto!important}";
  css += "  .dropdown-submenu>span::after{content:'\\25BC'!important}";
  css += "  .dropdown-submenu.active>span::after{content:'\\25B2'!important}";
  // Dropdown menu: constrain to screen width
  css += "  .dropdown-menu{max-width:90vw;right:0}";
  css += "}";
  css += "</style>";


  css += "<script>";
  css += "function updateThemeButton() {";
  css += "  var currentTheme = document.documentElement.getAttribute('data-theme') || 'light';";
  css += "  var themeButton = document.querySelector('.theme-toggle');";
  css += "  if (themeButton) {";
  css += "    themeButton.textContent = currentTheme === 'dark' ? '☀️' : '🌙';";
  css += "  }";
  css += "}";
  css += "var savedTheme = localStorage.getItem('theme') || 'light';";
  css += "document.documentElement.setAttribute('data-theme', savedTheme);";
  css += "document.addEventListener('DOMContentLoaded', function() {";
  css += "  updateThemeButton();";
  css += "});";
  css += "</script>";
  return css;
}

#endif // WEB_STYLES_H
