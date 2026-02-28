/*
 * web_handlers_snapshots.h — Configuration Snapshot Save/Load Routes
 *
 * Registers four API endpoints that let the user save and restore named
 * configuration snapshots to either the SD card (/config/) or internal
 * flash (LittleFS /config/).
 *
 * Routes:
 *   GET  /api/snapshots/list?storage=sd|flash
 *   POST /api/snapshots/save?storage=sd|flash&name=<filename>
 *   POST /api/snapshots/load?storage=sd|flash&name=<filename>
 *   POST /api/snapshots/delete?storage=sd|flash&name=<filename>
 */

#ifndef WEB_HANDLERS_SNAPSHOTS_H
#define WEB_HANDLERS_SNAPSHOTS_H

void registerSnapshotRoutes();

#endif // WEB_HANDLERS_SNAPSHOTS_H
