/*
 * admin.h - Admin Page Router for ESP32 MMDVM Hotspot Web Interface
 * Routes requests to modular admin page handlers
 */

#ifndef WEB_PAGES_ADMIN_H
#define WEB_PAGES_ADMIN_H

#include <Arduino.h>

// Include backend handler functions (POST/GET endpoints)
#include "../../include/handlers/admin/system_handlers.h"
#include "../../include/handlers/admin/settings_handlers.h"
#include "../../include/handlers/admin/security_handlers.h"
#include "../../include/handlers/admin/network_handlers.h"
#include "../../include/handlers/admin/maintenance_handlers.h"

// Include all modular admin page handlers (HTML generators)
#include "admin_common.h"
#include "admin_system.h"
#include "admin_settings.h"
#include "admin_security.h"
#include "admin_network.h"
#include "admin_maintenance.h"
#include "admin_all.h"

// External server reference
extern WebServer server;

/**
 * Admin page router - delegates to specific admin handlers based on submenu parameter
 * Query parameter: ?submenu=[system|settings|security|network|maintenance|all]
 * Default: system
 */
void handleAdmin() {
  if (!checkAuthentication()) return;

  // Get submenu parameter from URL, default to "system"
  String submenu = "system";
  if (server.hasArg("submenu")) {
    submenu = server.arg("submenu");
  }

  // Route to appropriate handler based on submenu
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
