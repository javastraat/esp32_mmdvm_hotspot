/*
 * web_handlers_wifi.h - WiFi Scan/Slot & NVS Utility Route Registration
 *
 * Declares registerWifiRoutes() which registers HTTP routes for:
 *  - /api/list-nvs-namespaces - list NVS namespaces (JSON)
 *  - /api/wifiscan            - scan WiFi networks (JSON)
 *  - /api/get-wifi-slot       - read one WiFi credential slot
 *  - /api/save-wifi-slot      - write one WiFi credential slot
 *  - /api/reset-wifi-slot     - reset one slot to config.h defaults
 *  - /api/prefs-reset         - erase the mmdvm NVS namespace
 */

#ifndef WEB_HANDLERS_WIFI_H
#define WEB_HANDLERS_WIFI_H

// Register WiFi scan/slot and NVS utility HTTP routes on the global web server instance.
// Must be called from webServerTask() after the WebServer object is reconstructed
// but before server.begin().
void registerWifiRoutes();

#endif // WEB_HANDLERS_WIFI_H
