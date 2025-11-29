/*
 * css.h - Common CSS and Theme for ESP32 MMDVM Hotspot Web Interface
 */

#ifndef WEB_COMMON_CSS_H
#define WEB_COMMON_CSS_H

#include <Arduino.h>

String getCommonCSS() {
  String css = "<style>";
  css += ":root { --bg-color: #f0f0f0; --container-bg: white; --text-color: #333; --border-color: #dee2e6; --card-bg: #f8f9fa; --info-bg: #e7f3ff; --topnav-bg: #333; --topnav-text: #f2f2f2; --topnav-hover: #ddd; --topnav-hover-text: black; --link-color: #007bff; --link-hover-color: #0056b3; }";
  css += "[data-theme='dark'] { --bg-color: #1a1a1a; --container-bg: #2d2d2d; --text-color: #ffffff; --border-color: #555; --card-bg: #3a3a3a; --info-bg: #1e3a5f; --topnav-bg: #000; --topnav-text: #f2f2f2; --topnav-hover: #444; --topnav-hover-text: #ffffff; --link-color: #4da6ff; --link-hover-color: #66b3ff; }";
  css += "body { font-family: Arial, sans-serif; margin: 0; background: var(--bg-color); color: var(--text-color); transition: background-color 0.3s, color 0.3s; }";
  css += ".topnav { background-color: var(--topnav-bg); overflow: hidden; }";
  css += ".topnav a { float: left; display: block; color: var(--topnav-text); text-align: center; padding: 14px 20px; text-decoration: none; }";
  css += ".topnav a:hover { background-color: var(--topnav-hover); color: var(--topnav-hover-text); }";
  css += ".topnav a.active { background-color: #007bff; color: white; }";
  css += ".theme-toggle { float: right; padding: 10px 15px; cursor: pointer; font-size: 18px; border: none; background: none; color: var(--topnav-text); }";
  css += ".theme-toggle:hover { background-color: var(--topnav-hover); color: var(--topnav-hover-text); border-radius: 4px; }";
  css += ".topnav .icon { display: none; }";
  css += "@media screen and (max-width: 600px) {";
  css += "  .topnav a:not(:first-child) {display: none;}";
  css += "  .topnav a.icon {float: right; display: block;}";
  css += "}";
  css += "@media screen and (max-width: 600px) {";
  css += "  .topnav.responsive {position: relative;}";
  css += "  .topnav.responsive .icon {position: absolute; right: 0; top: 0;}";
  css += "  .topnav.responsive a {float: none; display: block; text-align: left;}";
  css += "}";
  css += ".container { max-width: 1000px; margin: 20px auto; background: var(--container-bg); padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
  css += "h1 { color: var(--text-color); border-bottom: 2px solid #007bff; padding-bottom: 10px; margin-top: 0; }";
  css += "h2 { color: var(--text-color); margin-top: 30px; }";
  css += ".status { padding: 12px; margin: 10px 0; border-radius: 6px; font-weight: bold; }";
  css += ".status.connected { background: #d4edda; border: 1px solid #c3e6cb; color: #155724; }";
  css += ".status.disconnected { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; }";
  css += ".status.warning { background: #fff3cd; border: 1px solid #ffeaa7; color: #856404; }";
  css += ".info { padding: 12px; background: var(--info-bg); border-left: 4px solid #007bff; margin: 10px 0; border-radius: 0 4px 4px 0; }";
  css += ".grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 15px; margin: 20px 0; }";
  css += ".card { background: var(--card-bg); padding: 15px; border-radius: 6px; border: 1px solid var(--border-color); }";
  css += ".card h3 { margin-top: 0; color: var(--text-color); }";
  css += ".footer { text-align: center; padding: 20px 20px 5px 20px; margin-top: 30px; border-top: 1px solid var(--border-color); color: var(--text-color); font-size: 14px; }";
  css += ".footer2 { text-align: center; padding: 0px 20px 20px 20px; color: var(--text-color); font-size: 12px; }";
  css += ".footer2 a { color: var(--link-color); text-decoration: none; }";
  css += ".footer2 a:hover { color: var(--link-hover-color); text-decoration: underline; }";
  css += "p, div, span, strong, label { color: var(--text-color); }";
  css += ".metric-label, .metric-value { color: var(--text-color) !important; }";
  css += "input, select, textarea { background: var(--container-bg); color: var(--text-color); border: 1px solid var(--border-color); }";
  css += "input:focus, select:focus, textarea:focus { border-color: #007bff; }";
  css += "</style>";
  css += "<script>";
  css += "function updateThemeButton() {";
  css += "  var currentTheme = document.documentElement.getAttribute('data-theme') || 'light';";
  css += "  var themeButton = document.querySelector('.theme-toggle');";
  css += "  if (themeButton) {";
  css += "    themeButton.textContent = currentTheme === 'dark' ? 'Light' : 'Dark';";
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

#endif // WEB_COMMON_CSS_H
