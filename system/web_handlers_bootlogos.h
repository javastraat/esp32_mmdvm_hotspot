/*
 * web_handlers_bootlogos.h — Bootlogos Package Installer Routes
 *
 * Downloads bootlogos.zip from GitHub and extracts the .bin files
 * into /bootlogos on either LittleFS (internal flash) or SD card.
 *
 * Routes:
 *   POST /api/bootlogos/install?target=littlefs|sdcard
 *   GET  /api/bootlogos/status
 */

#ifndef WEB_HANDLERS_BOOTLOGOS_H
#define WEB_HANDLERS_BOOTLOGOS_H

void registerBootlogosRoutes();
void performBootlogosInstall();

#endif // WEB_HANDLERS_BOOTLOGOS_H
