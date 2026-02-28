/*
 * system_firmware.h - Firmware OTA Update Handler Declarations
 */

#ifndef SYSTEM_FIRMWARE_H
#define SYSTEM_FIRMWARE_H

#include <Arduino.h>
#include <FS.h>

// Firmware version strings
extern String firmwareVersion;
extern String modemFirmwareVersion;

// Parsed modem firmware fields (populated by parseModemFirmwareVersion())
extern String modemFwHardware;   // e.g. "MMDVM_HS_Hat"
extern String modemFwVersion;    // e.g. "v1.6.1"
extern String modemFwDate;       // e.g. "20260130"
extern String modemFwCrystal;    // e.g. "14.7456MHz"
extern String modemFwChip;       // e.g. "ADF7021"
extern String modemFwAuthors;    // e.g. "CA6JAU, G4KLX, PD2EMC"
extern String modemFwGitId;      // e.g. "#192cba3"

// Initialize firmware versions
void initFirmwareVersions();

// Parse modemFirmwareVersion into its component fields
void parseModemFirmwareVersion();

// Partition switch handler
void handleSwitchPartition(const char *targetLabel);

#endif // SYSTEM_FIRMWARE_H
