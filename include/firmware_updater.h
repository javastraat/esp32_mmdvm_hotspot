/*
 * firmware_updater.h - OTA Firmware Update Handlers
 */

#ifndef FIRMWARE_UPDATER_H
#define FIRMWARE_UPDATER_H

#include <Arduino.h>
#include <Update.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include "config.h"

// External references
extern String firmwareVersion;
extern String modemFirmwareVersion;

// Initialize firmware version strings
void initFirmwareVersions()
{
  firmwareVersion = FIRMWARE_VERSION;
  modemFirmwareVersion = "Unknown";
}

// Store partition version in NVS when firmware boots
void storeFirmwareVersion()
{
  const esp_partition_t *running = esp_ota_get_running_partition();
  if (running)
  {
    Preferences prefs;
    prefs.begin("mmdvm", false);
    String key = String("fw_") + String(running->label);
    prefs.putString(key.c_str(), FIRMWARE_VERSION);
    prefs.end();
  }
}

#endif // FIRMWARE_UPDATER_H
