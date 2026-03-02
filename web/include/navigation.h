/*
 * Navigation Bar Component with Dropdown Menu and Dark/Light Mode Toggle
 */

#ifndef WEB_NAVIGATION_H
#define WEB_NAVIGATION_H

#include <Arduino.h>

extern String userCallsign;
extern bool sdcardEnabled;

String getNavigation(String currentPage = "home", String currentTime = "") {
  String nav = getSharedStyles();
  // Navigation HTML
  nav += "<nav class='navbar'>";
  // Callsign button removed as per new design
  nav += "<div class='nav-menu'>";
  
  // Home link
  nav += "<div class='nav-item'>";
  nav += "<a href='/' class='nav-link" + String(currentPage == "home" ? " active" : "") + "'>Home</a>";
  nav += "</div>";
  
  // Dropdown menu
  nav += "<div class='nav-item dropdown' id='menuDropdown'>";
  nav += "<div class='nav-link dropdown-toggle' onclick='toggleDropdown()'>Menu</div>";
  nav += "<div class='dropdown-menu'>";
  nav += "<a href='/system-serialmonitor'>Serial Monitor</a>";
  nav += "<a href='/system-status'>System Status</a>";
  nav += "<a href='/system-info'>System Info</a>";
  nav += "<div class='dropdown-submenu' onclick='toggleSubmenu(event)'>";
  // Submenu Mode Config
  nav += "<span>Mode Config</span>";
  nav += "<div class='dropdown-submenu-menu'>";
  //nav += "<a href='/mode-select'>Mode Selection</a>";
  nav += "<a href='/mode-dmr'>DMR Mode</a>";
  nav += "<a href='/mode-dstar'>D-Star Mode</a>";
  nav += "<a href='/mode-ysf'>YSF Mode</a>";
  nav += "<a href='/mode-p25'>P25 Mode</a>";
  nav += "<a href='/mode-nxdn'>NXDN Mode</a>";
  nav += "<a href='/mode-pocsag'>POCSAG Mode</a>";
  nav += "</div>";
  nav += "</div>";
  //Submenu Network Config
  nav += "<div class='dropdown-submenu' onclick='toggleSubmenu(event)'>";
  nav += "<span>Network Config</span>";
  nav += "<div class='dropdown-submenu-menu'>";
  nav += "<a href='/system-wifi'>WiFi Config</a>";
  nav += "<a href='/system-wireguard'>WireGuard VPN</a>";
  nav += "</div>";
  nav += "</div>";
  //Submenu Message Config
  nav += "<div class='dropdown-submenu' onclick='toggleSubmenu(event)'>";
  nav += "<span>Messaging Config</span>";
  nav += "<div class='dropdown-submenu-menu'>";
  nav += "<a href='/system-mqtt'>MQTT Config</a>";
  nav += "<a href='/system-telegram'>Telegram Config</a>";
  nav += "</div>";
  nav += "</div>";
  

  nav += "<a href='/settings-mmdvm'>MMDVM Settings</a>";
  nav += "<a href='/system-settings'>Settings</a>";
  nav += "<a href='/system-wifi'>WiFi Config</a>";
  nav += "<a href='/system-mqtt'>MQTT Config</a>";
  nav += "<a href='/system-wireguard'>WireGuard VPN</a>";
  if (sdcardEnabled) {
    nav += "<a href='/system-sdcard'>SD Card</a>";
  }
  nav += "<a href='/system-firmware'>Firmware</a>";
  nav += "<a href='/system-admin'>Administration</a>";
  nav += "</div>";
  nav += "</div>";
  
  nav += "</div>";
  
  // Theme toggle button
  nav += "<button class='theme-toggle' onclick='toggleTheme()' title='Toggle Dark/Light Mode'>";
  nav += "<span id='themeIcon'>&#127769;</span>";  // Moon emoji
  nav += "</button>";
  
  nav += "</nav>";
  
  // JavaScript for dropdown and theme toggle
  nav += "<script>";
  
  // Dropdown toggle
  nav += "function toggleDropdown(){";
  nav += "document.getElementById('menuDropdown').classList.toggle('active');";
  nav += "}";
  nav += "document.addEventListener('click',function(e){";
  nav += "if(!e.target.closest('.dropdown')){document.getElementById('menuDropdown').classList.remove('active');}";
  nav += "});";
  
  // Submenu toggle
  nav += "function toggleSubmenu(e){";
  nav += "e.stopPropagation();";
  nav += "e.currentTarget.classList.toggle('active');";
  nav += "}";
  
  // Theme toggle
  nav += "function toggleTheme(){";
  nav += "const html=document.documentElement;";
  nav += "const currentTheme=html.getAttribute('data-theme');";
  nav += "const newTheme=currentTheme==='dark'?'light':'dark';";
  nav += "html.setAttribute('data-theme',newTheme);";
  nav += "localStorage.setItem('theme',newTheme);";
  nav += "var icon=document.getElementById('themeIcon'); if(icon) icon.innerHTML=newTheme==='dark'?'&#9728;':'&#127769;';";
  nav += "}";
  
  // Load saved theme on page load
  nav += "window.addEventListener('DOMContentLoaded',function(){";
  nav += "const savedTheme=localStorage.getItem('theme')||'light';";
  nav += "document.documentElement.setAttribute('data-theme',savedTheme);";
  nav += "var icon=document.getElementById('themeIcon'); if(icon) icon.innerHTML=savedTheme==='dark'?'&#9728;':'&#127769;';";
  nav += "});";

  // Highlight current page in dropdown menu (runs immediately - nav DOM already exists)
  nav += "(function(){";
  nav += "var path=window.location.pathname;";
  nav += "document.querySelectorAll('.dropdown-menu a, .dropdown-submenu-menu a').forEach(function(a){";
  nav += "if(a.getAttribute('href')===path) a.classList.add('active');";
  nav += "});";
  nav += "})();";
  
  nav += "</script>";
  
  return nav;
}

#endif // WEB_NAVIGATION_H
