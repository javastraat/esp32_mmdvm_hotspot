/*
 * admin_common.h - Common helper functions for all admin pages
 */

#ifndef WEB_PAGES_ADMIN_COMMON_H
#define WEB_PAGES_ADMIN_COMMON_H

#include <Arduino.h>
#include "css.h"
#include "navigation.h"
#include "utils.h"

// Common HTML header for all admin pages
static inline void appendAdminHeader(String& html, String pageTitle, String activePage) {
  html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>" + pageTitle + " - " + dmr_callsign + "</title>";
  
  // Inline CSS directly for better mobile performance
  html += "<style>";
  html += ":root{--bg-color:#f0f0f0;--container-bg:white;--text-color:#333;--border-color:#dee2e6;--card-bg:#f8f9fa;--info-bg:#e7f3ff;--topnav-bg:#333;--topnav-text:#f2f2f2;--topnav-hover:#ddd;--topnav-hover-text:black;--link-color:#007bff;--link-hover-color:#0056b3}";
  html += "[data-theme='dark']{--bg-color:#1a1a1a;--container-bg:#2d2d2d;--text-color:#fff;--border-color:#555;--card-bg:#3a3a3a;--info-bg:#1e3a5f;--topnav-bg:#000;--topnav-text:#f2f2f2;--topnav-hover:#444;--topnav-hover-text:#fff;--link-color:#4da6ff;--link-hover-color:#66b3ff}";
  html += "body{font-family:Arial,sans-serif;margin:0;background:var(--bg-color);color:var(--text-color)}";
  html += ".topnav{background-color:var(--topnav-bg);overflow:visible}.topnav a{float:left;display:block;color:var(--topnav-text);text-align:center;padding:14px 20px;text-decoration:none}";
  html += ".topnav a:hover{background-color:var(--topnav-hover);color:var(--topnav-hover-text)}.topnav a.active{background-color:#007bff;color:white}";
  html += ".theme-toggle{float:right;padding:10px 15px;cursor:pointer;font-size:18px;border:none;background:none;color:var(--topnav-text)}";
  html += ".dropdown{float:left;overflow:visible;position:relative}.dropdown .dropbtn{background-color:inherit;font-size:16px;border:none;color:var(--topnav-text);padding:14px 20px;cursor:pointer}";
  html += ".dropdown-content{display:none!important;position:absolute;background-color:var(--topnav-bg);min-width:200px;box-shadow:0 8px 16px 0 rgba(0,0,0,.2);z-index:1000}";
  html += ".dropdown-content a{float:none;color:var(--topnav-text);padding:12px 16px;text-decoration:none;display:block}.dropdown-content.show{display:block!important}";
  html += ".container{max-width:1000px;margin:20px auto;background:var(--container-bg);padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,.1)}";
  html += "h1{color:var(--text-color);border-bottom:2px solid #007bff;padding-bottom:10px;margin-top:0}h2{color:var(--text-color);margin-top:30px}";
  html += ".status{padding:12px;margin:10px 0;border-radius:6px;font-weight:bold}.status.connected{background:#d4edda;border:1px solid #c3e6cb;color:#155724}";
  html += ".status.disconnected{background:#f8d7da;border:1px solid #f5c6cb;color:#721c24}.status.warning{background:#fff3cd;border:1px solid #ffeaa7;color:#856404}";
  html += ".info{padding:12px;background:var(--info-bg);border-left:4px solid #007bff;margin:10px 0;border-radius:0 4px 4px 0}";
  html += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:15px;margin:20px 0}";
  html += ".card{background:var(--card-bg);padding:15px;border-radius:6px;border:1px solid var(--border-color)}.card h3{margin-top:0;color:var(--text-color)}";
  html += ".footer{text-align:center;padding:20px 20px 5px;margin-top:30px;border-top:1px solid var(--border-color);color:var(--text-color);font-size:14px}";
  html += ".footer2{text-align:center;padding:0 20px 20px;color:var(--text-color);font-size:12px}.footer2 a{color:var(--link-color);text-decoration:none}";
  html += "p,div,span,strong,label{color:var(--text-color)}.metric-label{color:var(--text-color)!important}.metric-value{color:var(--text-color)!important}";
  html += "input,select,textarea{background:var(--container-bg);color:var(--text-color);border:1px solid var(--border-color)}";
  html += "</style>";
  html += "<script>";
  html += "var t=localStorage.getItem('theme')||'light';document.documentElement.setAttribute('data-theme',t);";
  html += "document.addEventListener('DOMContentLoaded',function(){var b=document.querySelector('.theme-toggle');if(b)b.textContent=t==='dark'?'☀️':'🌙';});";
  html += "</script>";
  html += "<style>";
  html += ".admin-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 20px 0; }";
  html += "@media (max-width: 768px) { .admin-grid { grid-template-columns: 1fr; } }";  // Mobile: single column
  html += ".metric { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #eee; }";
  html += ".metric:last-child { border-bottom: none; }";
  html += ".metric-label { font-weight: bold; color: #555; }";
  html += ".metric-value { color: #333; }";
  html += ".uptime { color: #007bff; font-weight: bold; }";
  html += ".btn { display: inline-block; padding: 12px 24px; margin: 10px 5px; border: none; border-radius: 6px; cursor: pointer; text-decoration: none; font-size: 14px; font-weight: bold; text-align: center; transition: background-color 0.3s; }";
  html += ".btn-primary { background: #007bff; color: white; }";
  html += ".btn-primary:hover { background: #0056b3; }";
  html += ".btn-success { background: #28a745; color: white; }";
  html += ".btn-success:hover { background: #218838; }";
  html += ".btn-warning { background: #ffc107; color: black; }";
  html += ".btn-warning:hover { background: #e0a800; }";
  html += ".btn-danger { background: #dc3545; color: white; }";
  html += ".btn-danger:hover { background: #c82333; }";
  html += ".btn-info { background: #17a2b8; color: white; }";
  html += ".btn-info:hover { background: #138496; }";
  html += ".action-buttons { text-align: center; margin: 15px 0; }";
  html += ".action-buttons-vertical { text-align: center; margin: 15px 0; }";
  html += ".action-buttons-vertical .btn { display: block; margin: 8px auto; width: 80%; }";
  html += "</style></head><body>";
  
  html += getNavigation(activePage);
  html += "<div class='container'>";
  html += "<h1>" + pageTitle + "</h1>";
}

// Common HTML footer for all admin pages
static inline void appendAdminFooter(String& html) {
  html += "</div>"; // Close container
  html += getFooter();
  html += "</body></html>";
}

#endif // WEB_PAGES_ADMIN_COMMON_H
