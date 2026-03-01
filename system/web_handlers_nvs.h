/*
 * web_handlers_nvs.h - NVS Preferences Viewer/Repair Route Registration
 *
 * Declares registerNvsRoutes() which registers HTTP routes for:
 *  - /api/show-prefs      - formatted NVS viewer (HTML table, all known keys)
 *  - /api/show-prefs-raw  - raw NVS enumeration via ESP-IDF iterator (HTML)
 *  - /api/repair-prefs    - add any missing NVS keys with config.h defaults
 */

#ifndef WEB_HANDLERS_NVS_H
#define WEB_HANDLERS_NVS_H

// Register NVS viewer/repair HTTP routes on the global web server instance.
// Must be called from webServerTask() after the WebServer object is reconstructed
// but before server.begin().
void registerNvsRoutes();

#endif // WEB_HANDLERS_NVS_H
