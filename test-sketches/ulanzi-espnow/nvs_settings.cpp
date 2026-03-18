// nvs_settings.cpp — NVS Preferences: load and save all user-configurable settings.
#include "nvs_settings.h"
#include "globals.h"
#include <Preferences.h>

void loadSettings() {
  Preferences p;
  p.begin("ulanzi", true);  // read-only
  // Brightness
  autoBrightnessEnabled = p.getBool ("auto_br",    true);
  currentBrightness     = p.getUChar("brightness", LED_BRIGHTNESS);
  // Buzzer
  buzzerBootEnabled     = p.getBool ("boot_en",   true);
  buzzerBootVolume      = p.getUChar("boot_vol",  BUZZER_VOL_BOOT);
  buzzerPocsagEnabled   = p.getBool ("poc_en",    true);
  buzzerPocsagVolume    = p.getUChar("poc_vol",   BUZZER_VOL_POCSAG);
  buzzerClickEnabled    = p.getBool ("clk_en",    true);
  buzzerClickVolume     = p.getUChar("clk_vol",   BUZZER_VOL_CLICK);
  // Display rotation
  autoRotateEnabled     = p.getBool ("rot_en",  false);
  autoRotateIntervalSec = p.getUChar("rot_sec", 5);
  // Icon filenames — trim whitespace to fix any accidentally saved leading/trailing spaces
  { String s = p.getString("icon_temp", "/icons/70122.jpg");  s.trim(); strncpy(iconTempFile,   s.c_str(), 31); iconTempFile[31]   = '\0'; }
  { String s = p.getString("icon_hum",  "/icons/71006.jpg");  s.trim(); strncpy(iconHumFile,    s.c_str(), 31); iconHumFile[31]    = '\0'; }
  { String s = p.getString("icon_bat",  "/icons/390.jpg");    s.trim(); strncpy(iconBatFile,    s.c_str(), 31); iconBatFile[31]    = '\0'; }
  { String s = p.getString("icon_poc",  "/icons/18675.jpg");  s.trim(); strncpy(iconPocsagFile, s.c_str(), 31); iconPocsagFile[31] = '\0'; }
  // Screensaver
  screensaverEnabled    = p.getBool   ("ss_en",      false);
  screensaverTimeoutSec = p.getUShort ("ss_timeout", 60);
  { String s = p.getString("ss_file", ""); s.trim(); strncpy(screensaverFile, s.c_str(), 63); screensaverFile[63] = '\0'; }
  p.end();
}

void saveSettings() {
  Preferences p;
  p.begin("ulanzi", false);  // read-write
  // Brightness
  p.putBool ("auto_br",    autoBrightnessEnabled);
  p.putUChar("brightness", currentBrightness);
  // Buzzer
  p.putBool ("boot_en",  buzzerBootEnabled);
  p.putUChar("boot_vol", buzzerBootVolume);
  p.putBool ("poc_en",   buzzerPocsagEnabled);
  p.putUChar("poc_vol",  buzzerPocsagVolume);
  p.putBool ("clk_en",   buzzerClickEnabled);
  p.putUChar("clk_vol",  buzzerClickVolume);
  // Display rotation
  p.putBool ("rot_en",  autoRotateEnabled);
  p.putUChar("rot_sec", autoRotateIntervalSec);
  // Icon filenames
  p.putString("icon_temp", iconTempFile);
  p.putString("icon_hum",  iconHumFile);
  p.putString("icon_bat",  iconBatFile);
  p.putString("icon_poc",  iconPocsagFile);
  // Screensaver
  p.putBool   ("ss_en",      screensaverEnabled);
  p.putUShort ("ss_timeout", screensaverTimeoutSec);
  p.putString ("ss_file",    screensaverFile);
  p.end();
}
