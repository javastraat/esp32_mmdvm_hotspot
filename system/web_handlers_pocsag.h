/*
 * web_handlers_pocsag.h - POCSAG/DAPNET Route Registration
 *
 * Declares registerPocsagRoutes() which registers HTTP routes for:
 *  - /api/send-pocsag        - send a POCSAG test message
 *  - /api/save-pocsag-settings  - POCSAG/DAPNET settings
 *  - /api/reset-pocsag-settings
 *  - /api/pocsag-queue       - current POCSAG queue contents (JSON)
 *  - /api/dapnet-history     - recent DAPNET messages (JSON)
 */

#ifndef WEB_HANDLERS_POCSAG_H
#define WEB_HANDLERS_POCSAG_H

// Register POCSAG/DAPNET HTTP routes on the global web server instance.
// Must be called from webServerTask() after the WebServer object is reconstructed
// but before server.begin().
void registerPocsagRoutes();

#endif // WEB_HANDLERS_POCSAG_H
