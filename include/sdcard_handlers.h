/*
 * sdcard_handlers.h - SD Card API Handlers
 *
 * API endpoints for SD card management, database downloads, and DMR searches
 */

#ifndef SDCARD_HANDLERS_H
#define SDCARD_HANDLERS_H

#include <Arduino.h>
#include <WebServer.h>
#include <SD.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include "config.h"
#include "system/system_sdcard.h"

// External references
extern WebServer server;
extern Preferences preferences;

// Download progress tracking
extern volatile bool csvDownloadActive;
extern volatile int csvDownloadProgress;
extern volatile unsigned long csvBytesTotal;
extern volatile unsigned long csvBytesWritten;
extern String csvDownloadStatus;

extern volatile bool sqliteDownloadActive;
extern volatile int sqliteDownloadProgress;
extern volatile unsigned long sqliteBytesTotal;
extern volatile unsigned long sqliteBytesWritten;
extern String sqliteDownloadStatus;

// Update checking
extern unsigned long csvRemoteSize;
extern unsigned long csvLocalSize;
extern bool csvUpdateAvailable;
extern unsigned long sqliteRemoteSize;
extern unsigned long sqliteLocalSize;
extern bool sqliteUpdateAvailable;

// ===== Helper Functions =====

// List files recursively
void listFilesRecursive(File dir, String &output, int indent = 0);

// Delete recursively
void deleteRecursiveSD(const char *path);

// Check for database updates
bool checkForCSVUpdate();
bool checkForSQLiteUpdate();

// JSON escape helper
String jsonEscape(const String &s);

// ===== API Handlers =====

// GET /api/sdcard/status - SD card availability and basic info
void handleSDCardStatus();

// GET /api/sdcard/info - Get SD card and database info
void handleSDCardInfo();

// GET /api/sdcard/files - List files on SD card
void handleSDCardFiles();

// GET /api/sdcard/owner - Get owner.txt content
void handleSDCardOwner();

// POST /api/sdcard/writeowner - Write owner.txt
void handleWriteOwner();

// GET /api/sdcard/download/csv - Start CSV database download
void handleDownloadCSV();

// GET /api/sdcard/download/sqlite - Start SQLite database download
void handleDownloadSQLite();

// GET /api/sdcard/status/csv - Get CSV download status
void handleCSVDownloadStatus();

// GET /api/sdcard/status/sqlite - Get SQLite download status
void handleSQLiteDownloadStatus();

// GET /api/sdcard/delete/csv - Delete CSV database
void handleDeleteCSV();

// GET /api/sdcard/delete/sqlite - Delete SQLite database
void handleDeleteSQLite();

// GET /api/sdcard/delete/custom?path=/path - Delete custom path
void handleDeleteCustomPath();

// GET /api/dmr/user/?id=1234567 - Search CSV database by Radio ID
void handleDMRUserSearch();

// GET /api/sqlite/search?field=radio_id&value=1234567 - Search SQLite database
void handleSQLiteSearch();

// ===== Download Processing Functions =====

// Call these from main loop to process download requests
void performCSVDownload();
void performSQLiteDownload();

// ===== Setup Function =====

// Register all SD card API handlers
void setupSDCardHandlers(WebServer &server);

#endif // SDCARD_HANDLERS_H
