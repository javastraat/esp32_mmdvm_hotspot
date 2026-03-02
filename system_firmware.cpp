/*
 * system_firmware.cpp - Firmware OTA Update Handlers
 */

#include "system/system_firmware.h"
#include "system/system_logger.h"
#include "system/service_mqtt.h"
#include "include/config.h"
#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <Preferences.h>

extern String mqttArduinoOtaTaskTopic; // MQTT topic for firmware task status updates

// Global firmware version strings
String firmwareVersion = FIRMWARE_VERSION;
String modemFirmwareVersion = "Unknown";

// Parsed modem firmware fields (populated by parseModemFirmwareVersion())
String modemFwHardware = "";
String modemFwVersion  = "";
String modemFwDate     = "";
String modemFwCrystal  = "";
String modemFwChip     = "";
String modemFwAuthors  = "";
String modemFwGitId    = "";

/*
 * Parse modemFirmwareVersion into its component fields.
 *
 * Expected format:
 *   HARDWARE-vVERSION DATE CRYSTAL CHIP FW by AUTHORS GitID #GITID
 * Example:
 *   MMDVM_HS_Hat-v1.6.1 20260130 14.7456MHz ADF7021 FW by CA6JAU, G4KLX, PD2EMC GitID #192cba3
 */
void parseModemFirmwareVersion()
{
  // Reset fields
  modemFwHardware = modemFwVersion = modemFwDate = "";
  modemFwCrystal  = modemFwChip   = modemFwAuthors = modemFwGitId = "";

  const String &s = modemFirmwareVersion;
  if (s.length() == 0 || s == "Unknown") return;

  // ---- Token 0: HARDWARE-vVERSION ----
  int sp0 = s.indexOf(' ');
  String tok0 = (sp0 >= 0) ? s.substring(0, sp0) : s;
  // Split at last '-' before 'v' (handles underscores in hardware name)
  int dashV = tok0.lastIndexOf("-v");
  if (dashV >= 0)
  {
    modemFwHardware = tok0.substring(0, dashV);
    modemFwVersion  = tok0.substring(dashV + 1); // includes 'v'
  }
  else
  {
    modemFwHardware = tok0; // no version marker
  }

  if (sp0 < 0) return; // no more tokens

  // ---- Remaining tokens ----
  String rest = s.substring(sp0 + 1);

  // Extract GitID (#...) from the end
  int gitIdx = rest.indexOf("GitID ");
  if (gitIdx >= 0)
  {
    modemFwGitId = rest.substring(gitIdx + 6); // after "GitID "
    modemFwGitId.trim();
    rest = rest.substring(0, gitIdx);
    rest.trim();
  }

  // Extract authors (after "FW by ")
  int fwByIdx = rest.indexOf("FW by ");
  if (fwByIdx >= 0)
  {
    modemFwAuthors = rest.substring(fwByIdx + 6);
    modemFwAuthors.trim();
    rest = rest.substring(0, fwByIdx);
    rest.trim();
  }

  // Remaining: DATE CRYSTAL CHIP (space-separated)
  int p = 0;
  int field = 0;
  while (p < (int)rest.length())
  {
    int next = rest.indexOf(' ', p);
    String tok = (next >= 0) ? rest.substring(p, next) : rest.substring(p);
    tok.trim();
    if (tok.length() > 0)
    {
      if (field == 0)      modemFwDate    = tok;
      else if (field == 1) modemFwCrystal = tok;
      else if (field == 2) modemFwChip    = tok;
      field++;
    }
    if (next < 0) break;
    p = next + 1;
  }
}

// Initialize firmware versions
void initFirmwareVersions()
{
  firmwareVersion = FIRMWARE_VERSION;

  // Pre-populate modem firmware from last successful detection (NVS cache).
  // Will be overwritten once the modem actually responds during setupMMDVM().
  {
    Preferences prefs;
    prefs.begin("mmdvm", true); // read-only
    String cached = prefs.getString("modem_fw", "");
    prefs.end();
    modemFirmwareVersion = (cached.length() > 0) ? cached : "Unknown";
  }

  // Store current ESP firmware version in NVS (only if changed)
  const esp_partition_t *running = esp_ota_get_running_partition();
  if (running)
  {
    Preferences prefs;
    prefs.begin("mmdvm", false);
    String key = String("fw_") + String(running->label);
    if (prefs.getString(key.c_str(), "") != firmwareVersion)
      prefs.putString(key.c_str(), firmwareVersion.c_str());
    prefs.end();
  }
}

// Handle partition switch
void handleSwitchPartition(const char *targetLabel)
{
  addLogMessage("[OTA Task] Switching to partition: " + String(targetLabel));
  publishMqtt(mqttArduinoOtaTaskTopic.c_str(),
              "{\"event\":\"switching_partition\",\"target\":\"" + String(targetLabel) + "\"}");

  const esp_partition_t *target = nullptr;

  if (strcmp(targetLabel, "app0") == 0)
  {
    target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
  }
  else if (strcmp(targetLabel, "app1") == 0)
  {
    target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
  }

  if (target == nullptr)
  {
    addLogMessage("[OTA Task] Error: Partition not found");
    publishMqtt(mqttArduinoOtaTaskTopic.c_str(),
                "{\"event\":\"error\",\"reason\":\"partition_not_found\",\"target\":\"" + String(targetLabel) + "\"}");
    return;
  }

  // Set boot partition
  esp_err_t err = esp_ota_set_boot_partition(target);
  if (err == ESP_OK)
  {
    addLogMessage("[OTA Task] Boot partition set to " + String(targetLabel));
    publishMqtt(mqttArduinoOtaTaskTopic.c_str(),
                "{\"event\":\"partition_set\",\"target\":\"" + String(targetLabel) + "\"}");
    addLogMessage("[OTA Task] Rebooting...");
    publishMqtt(mqttArduinoOtaTaskTopic.c_str(), "{\"event\":\"rebooting\"}");
    delay(1000);
    ESP.restart();
  }
  else
  {
    addLogMessage("[OTA Task] Error setting boot partition: " + String(err));
    publishMqtt(mqttArduinoOtaTaskTopic.c_str(),
                "{\"event\":\"error\",\"reason\":\"set_partition_failed\",\"code\":" + String(err) + "}");
  }
}