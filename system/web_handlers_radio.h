/*
 * web_handlers_radio.h - POCSAG/DMR Old-Style & WiFi/NVS Utility Route Registration
 *
 * Declares registerRadioRoutes() which registers HTTP routes for:
 *  - POCSAG send, settings, queue and DAPNET history
 *  - DMR general/network/hotspot/mode old-style handlers
 *  - NVS namespace list, WiFi scan
 *  - WiFi credential slot management
 *  - NVS preferences reset
 */

#ifndef WEB_HANDLERS_RADIO_H
#define WEB_HANDLERS_RADIO_H

// Register all radio/WiFi/NVS utility HTTP routes on the global web server instance.
// Must be called from webServerTask() after the WebServer object is reconstructed
// but before server.begin().
void registerRadioRoutes();

#endif // WEB_HANDLERS_RADIO_H
