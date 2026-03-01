/*
 * web_handlers_admin.h - Admin & System API Route Registration
 *
 * Declares registerAdminRoutes() which registers all HTTP routes related to:
 *  - Log access and management (/api/logs, /api/logs/clear, /api/status)
 *  - Configuration export/import (/api/export-config, /api/import-config)
 *  - NVS preferences viewer (/api/show-prefs, /api/show-prefs-raw, /api/repair-prefs)
 *  - System control (factory-reset, restart-mmdvm, restart-mqtt, restart-services)
 */

#ifndef WEB_HANDLERS_ADMIN_H
#define WEB_HANDLERS_ADMIN_H

// Register all admin / system-control HTTP routes on the global web server instance.
// Must be called from webServerTask() after the WebServer object is reconstructed
// but before server.begin().
void registerAdminRoutes();

#endif // WEB_HANDLERS_ADMIN_H
