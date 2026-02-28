/*
 * web_handlers_ota.h - OTA & Modem Flash Route Registration
 *
 * Declares registerOtaRoutes() which registers all HTTP routes related to:
 *  - ESP32 firmware OTA (download-update, upload-firmware, flash-firmware, cancel-flash)
 *  - Partition management (switch-partition)
 *  - MMDVM modem firmware flashing (flash-modem-url, flash-modem-upload)
 *  - Modem serial settings (save-modem-settings, reset-modem-settings, get-modem-version)
 */

#ifndef WEB_HANDLERS_OTA_H
#define WEB_HANDLERS_OTA_H

// Register all OTA / modem-flash HTTP routes on the global web server instance.
// Must be called from webServerTask() after the WebServer object is reconstructed
// but before server.begin().
void registerOtaRoutes();

#endif // WEB_HANDLERS_OTA_H
