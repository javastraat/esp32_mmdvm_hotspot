/*
 * admin_common.h - Common helper functions for all admin pages
 */

#ifndef WEB_PAGES_ADMIN_COMMON_H
#define WEB_PAGES_ADMIN_COMMON_H

#include <Arduino.h>

// Common HTML header for all admin pages
String getAdminHeader(String pageTitle, String activePage) {
  String html;
  html.reserve(4000);
  
  html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>" + pageTitle + " - " + dmr_callsign + "</title>";
  html += getCommonCSS();
  html += "<style>";
  html += ".admin-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 20px 0; }";
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
  
  return html;
}

// Common HTML footer for all admin pages
String getAdminFooter() {
  String html;
  html += "</div>"; // Close container
  html += getFooter();
  html += "</body></html>";
  return html;
}

#endif // WEB_PAGES_ADMIN_COMMON_H
