/*
 * navigation.h - Navigation Bar for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_COMMON_NAVIGATION_H
#define WEB_COMMON_NAVIGATION_H

#include <Arduino.h>

String getNavigation(String activePage) {
  String nav = "<div class='topnav' id='myTopnav'>";
  nav += "<a href='/'" + String(activePage == "main" ? " class='active'" : "") + ">Main</a>";
  nav += "<a href='/status'" + String(activePage == "status" ? " class='active'" : "") + ">Status</a>";
  nav += "<a href='/serialmonitor'" + String(activePage == "monitor" ? " class='active'" : "") + ">Serial Monitor</a>";
  nav += "<a href='/wificonfig'" + String(activePage == "wificonfig" ? " class='active'" : "") + ">WiFi Config</a>";
  nav += "<a href='/modeconfig'" + String(activePage == "modeconfig" ? " class='active'" : "") + ">Mode Config</a>";
  nav += "<a href='/admin'" + String(activePage == "admin" ? " class='active'" : "") + ">Admin</a>";
  nav += "<button class='theme-toggle' onclick='toggleTheme()' title='Toggle Dark/Light Mode' id='theme-btn'>Dark</button>";
  nav += "<script>";
  nav += "var savedTheme = localStorage.getItem('theme') || 'light';";
  nav += "var themeBtn = document.getElementById('theme-btn');";
  nav += "if (themeBtn) themeBtn.textContent = savedTheme === 'dark' ? 'Light' : 'Dark';";
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
  nav += "</script>";
  return nav;
}

#endif // WEB_COMMON_NAVIGATION_H
