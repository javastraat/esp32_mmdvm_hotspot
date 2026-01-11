/*
 * navigation.h - Navigation Bar for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_COMMON_NAVIGATION_H
#define WEB_COMMON_NAVIGATION_H

#include <Arduino.h>

String getNavigation(String activePage) {
  // Check if active page is any admin submenu
  bool isAdminActive = (activePage == "admin" || activePage == "admin-system" || 
                         activePage == "admin-settings" || activePage == "admin-security" || 
                         activePage == "admin-network" || activePage == "admin-maintenance" ||
                         activePage == "admin-all");
  
  String nav = "<div class='topnav' id='myTopnav'>";
  nav += "<a href='/'" + String(activePage == "main" ? " class='active'" : "") + ">Main</a>";
  nav += "<a href='/status'" + String(activePage == "status" ? " class='active'" : "") + ">Status</a>";
  // nav += "<a href='/serialmonitor'" + String(activePage == "monitor" ? " class='active'" : "") + ">Serial Monitor</a>";
  // nav += "<a href='/wificonfig'" + String(activePage == "wificonfig" ? " class='active'" : "") + ">WiFi Config</a>";
  // nav += "<a href='/modeconfig'" + String(activePage == "modeconfig" ? " class='active'" : "") + ">Mode Config</a>";
  
  // Admin dropdown menu
  nav += "<div class='dropdown'>";
  nav += "<button type='button' onclick='toggleDropdown(event); return false;' class='dropbtn" + String(isAdminActive ? " active" : "") + "'>Admin ▼</button>";
  nav += "<div class='dropdown-content' id='adminDropdown'>";
  nav += "<a href='/admin_system'" + String(activePage == "admin-system" ? " style='background-color:var(--topnav-hover);'" : "") + ">System</a>";
  nav += "<a href='/admin_settings'" + String(activePage == "admin-settings" ? " style='background-color:var(--topnav-hover);'" : "") + ">Settings</a>";
  nav += "<a href='/wificonfig'" + String(activePage == "wificonfig" ? " style='background-color:var(--topnav-hover);'" : "") + ">WiFi Config</a>";
  nav += "<a href='/modeconfig'" + String(activePage == "modeconfig" ? " style='background-color:var(--topnav-hover);'" : "") + ">Mode Config</a>";
  nav += "<a href='/admin_network'" + String(activePage == "admin-network" ? " style='background-color:var(--topnav-hover);'" : "") + ">Network</a>";
  nav += "<a href='/admin_security'" + String(activePage == "admin-security" ? " style='background-color:var(--topnav-hover); border-top: 1px solid var(--border-color); margin-top: 5px; padding-top: 5px;'" : "") + " style='border-top: 1px solid var(--border-color); margin-top: 5px; padding-top: 5px;'>Security</a>";
  nav += "<a href='/admin_maintenance'" + String(activePage == "admin-maintenance" ? " style='background-color:var(--topnav-hover);'" : "") + ">Maintenance</a>";
  nav += "<a href='/serialmonitor'" + String(activePage == "monitor" ? " style='background-color:var(--topnav-hover); border-top: 1px solid var(--border-color); margin-top: 5px; padding-top: 5px;'" : "") + " style='border-top: 1px solid var(--border-color); margin-top: 5px; padding-top: 5px;'>Serial Monitor</a>";
  //nav += "<a href='/admin_all' style='border-top: 1px solid var(--border-color); margin-top: 5px; padding-top: 5px;" + String(activePage == "admin-all" ? " background-color:var(--topnav-hover);" : "") + "'>All Sections</a>";
  nav += "</div>";
  nav += "</div>";
  
  nav += "<button class='theme-toggle' onclick='toggleTheme()' title='Toggle Dark/Light Mode' id='theme-btn'>🌙</button>";
  nav += "<script>";
  nav += "var savedTheme = localStorage.getItem('theme') || 'light';";
  nav += "var themeBtn = document.getElementById('theme-btn');";
  nav += "if (themeBtn) themeBtn.textContent = savedTheme === 'dark' ? '☀️' : '🌙';";
  nav += "</script>";
  nav += "<a href='javascript:void(0);' class='icon' onclick='toggleNav()'>&#9776;</a>";
  nav += "</div>";
  nav += "<script>";
  nav += "function toggleNav() {";
  nav += "  var x = document.getElementById('myTopnav');";
  nav += "  if (x.className === 'topnav') {";
  nav += "    x.className += ' responsive';";
  nav += "  } else {";
  nav += "    x.className = 'topnav';";
  nav += "  }";
  nav += "}";
  nav += "function toggleTheme() {";
  nav += "  var currentTheme = document.documentElement.getAttribute('data-theme') || 'light';";
  nav += "  var newTheme = currentTheme === 'dark' ? 'light' : 'dark';";
  nav += "  document.documentElement.setAttribute('data-theme', newTheme);";
  nav += "  localStorage.setItem('theme', newTheme);";
  nav += "  updateThemeButton();";
  nav += "}";
  nav += "window.toggleDropdown = function(event) {";
  nav += "  if (event) { event.preventDefault(); event.stopPropagation(); }";
  nav += "  var dropdown = document.getElementById('adminDropdown');";
  nav += "  if (dropdown) { dropdown.classList.toggle('show'); }";
  nav += "};";
  nav += "window.onclick = function(event) {";
  nav += "  if (!event.target.matches('.dropbtn')) {";
  nav += "    var dropdowns = document.getElementsByClassName('dropdown-content');";
  nav += "    for (var i = 0; i < dropdowns.length; i++) {";
  nav += "      var openDropdown = dropdowns[i];";
  nav += "      if (openDropdown.classList.contains('show')) {";
  nav += "        openDropdown.classList.remove('show');";
  nav += "      }";
  nav += "    }";
  nav += "  }";
  nav += "}";
  nav += "</script>";
  return nav;
}

#endif // WEB_COMMON_NAVIGATION_H
