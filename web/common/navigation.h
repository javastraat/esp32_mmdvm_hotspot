/*
 * navigation.h - Navigation Bar for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_COMMON_NAVIGATION_H
#define WEB_COMMON_NAVIGATION_H

#include <Arduino.h>

String getNavigation(String activePage) {
  String nav = "<style>";
  nav += ".dropdown { float: left; position: relative; }";
  nav += ".dropdown-content { display: none; position: fixed; background-color: var(--topnav-bg, #333); min-width: 160px; box-shadow: 0px 8px 16px 0px rgba(0,0,0,0.2); z-index: 1000; }";
  nav += ".dropdown-content a { float: none !important; color: var(--topnav-text, white); padding: 12px 16px; text-decoration: none; display: block !important; text-align: left; }";
  nav += ".dropdown-content a:hover { background-color: var(--topnav-hover, #575757); }";
  nav += ".dropdown-content.show { display: block; }";
  nav += ".dropbtn { background-color: transparent; color: var(--topnav-text, white); padding: 14px 20px; font-size: inherit; border: none; cursor: pointer; font-family: inherit; }";
  nav += ".dropbtn:hover { background-color: var(--topnav-hover, #575757); }";
  nav += "</style>";
  nav += "<div class='topnav' id='myTopnav'>";
  nav += "<a href='/'" + String(activePage == "main" ? " class='active'" : "") + ">Main</a>";
  nav += "<a href='/status'" + String(activePage == "status" ? " class='active'" : "") + ">Status</a>";
  //nav += "<a href='/serialmonitor'" + String(activePage == "monitor" ? " class='active'" : "") + ">Serial Monitor</a>";
  //nav += "<a href='/wificonfig'" + String(activePage == "wificonfig" ? " class='active'" : "") + ">WiFi Config</a>";
  //nav += "<a href='/modeconfig'" + String(activePage == "modeconfig" ? " class='active'" : "") + ">Mode Config</a>";
  nav += "<a href='/admin'" + String(activePage == "admin" ? " class='active'" : "") + ">Admin</a>";
  nav += "<div class='dropdown'>";
  nav += "<button class='dropbtn' onclick='toggleDropdown(event)'" + String(activePage == "admin2" ? " style='background-color:var(--active-bg,#04AA6D);'" : "") + ">Admin2 &#9662;</button>";
  nav += "<div class='dropdown-content' id='adminDropdown'>";
  //nav += "<a href='/wificonfig'>WiFi Config</a>";
  //nav += "<a href='/modeconfig'>Mode Config</a>";
  //nav += "<a href='/serialmonitor'>Monitor</a>";
  nav += "<a href='/serialmonitor'" + String(activePage == "monitor" ? " class='active'" : "") + ">Serial Monitor</a>";
  nav += "<a href='/wificonfig'" + String(activePage == "wificonfig" ? " class='active'" : "") + ">WiFi Config</a>";
  nav += "<a href='/modeconfig'" + String(activePage == "modeconfig" ? " class='active'" : "") + ">Mode Config</a>";
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
  nav += "function toggleDropdown(e) {";
  nav += "  var dropdown = document.getElementById('adminDropdown');";
  nav += "  var btn = e.target;";
  nav += "  var rect = btn.getBoundingClientRect();";
  nav += "  dropdown.style.top = rect.bottom + 'px';";
  nav += "  dropdown.style.left = rect.left + 'px';";
  nav += "  dropdown.classList.toggle('show');";
  nav += "}";
  nav += "window.onclick = function(e) {";
  nav += "  if (!e.target.matches('.dropbtn')) {";
  nav += "    var dd = document.getElementById('adminDropdown');";
  nav += "    if (dd && dd.classList.contains('show')) dd.classList.remove('show');";
  nav += "  }";
  nav += "}";
  nav += "</script>";
  return nav;
}

#endif // WEB_COMMON_NAVIGATION_H
