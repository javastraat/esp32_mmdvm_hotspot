/*
 * web_handlers_config.h - Configuration Export/Import Route Registration
 *
 * Declares registerConfigRoutes() which registers HTTP routes for:
 *  - /api/export-config  - dump all settings as key=value text file
 *  - /api/import-config  - restore settings from key=value text
 */

#ifndef WEB_HANDLERS_CONFIG_H
#define WEB_HANDLERS_CONFIG_H

#include <Arduino.h>

// Register config export/import HTTP routes on the global web server instance.
// Must be called from webServerTask() after the WebServer object is reconstructed
// but before server.begin().
void registerConfigRoutes();

// Shared helpers — also used by the snapshot save/load handler.
String generateConfigString();          // serialize all current settings → key=value text
int    applyConfigString(const String& body); // parse key=value text → globals, returns count

#endif // WEB_HANDLERS_CONFIG_H
