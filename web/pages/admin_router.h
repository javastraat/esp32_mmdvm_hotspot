/*
 * admin.h - Admin Page Router
 * Routes to appropriate admin subpage based on submenu parameter
 */

#ifndef WEB_PAGES_ADMIN_H
#define WEB_PAGES_ADMIN_H

#include <Arduino.h>

void handleAdmin() {
  if (!checkAuthentication()) return;

  // Get submenu parameter from URL
  String submenu = "system";  // Default to system page
  if (server.hasArg("submenu")) {
    submenu = server.arg("submenu");
  }

  // Route to appropriate handler
  if (submenu == "system") {
    handleAdminSystem();
  } else if (submenu == "settings") {
    handleAdminSettings();
  } else if (submenu == "security") {
    handleAdminSecurity();
  } else if (submenu == "network") {
    handleAdminNetwork();
  } else if (submenu == "maintenance") {
    handleAdminMaintenance();
  } else if (submenu == "all") {
    handleAdminAll();
  } else {
    // Default to system if unknown submenu
    handleAdminSystem();
  }
}

#endif // WEB_PAGES_ADMIN_H
