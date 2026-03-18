// nvs_settings.cpp — NVS Preferences: load and save all user-configurable settings.
#include "nvs_settings.h"
#include "globals.h"
#include <Preferences.h>

static inline uint32_t packCRGB(CRGB c)       { return ((uint32_t)c.r<<16)|((uint32_t)c.g<<8)|c.b; }
static inline CRGB     unpackCRGB(uint32_t v) { return CRGB((v>>16)&0xFF,(v>>8)&0xFF,v&0xFF); }

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
  // Display colors
  colorClock   = unpackCRGB(p.getUInt("col_clock",  packCRGB(CRGB(255,255,255))));
  colorPocsag  = unpackCRGB(p.getUInt("col_poc",    packCRGB(CRGB(255,160,  0))));
  tempThreshLo = p.getFloat ("t_thr_lo", 15.0f);
  tempThreshHi = p.getFloat ("t_thr_hi", 28.0f);
  colorTempLo  = unpackCRGB(p.getUInt("t_col_lo",  packCRGB(CRGB(  0,160,255))));
  colorTempMid = unpackCRGB(p.getUInt("t_col_mid", packCRGB(CRGB(  0,200, 50))));
  colorTempHi  = unpackCRGB(p.getUInt("t_col_hi",  packCRGB(CRGB(255, 80,  0))));
  humThreshLo  = p.getFloat ("h_thr_lo", 40.0f);
  humThreshHi  = p.getFloat ("h_thr_hi", 70.0f);
  colorHumLo   = unpackCRGB(p.getUInt("h_col_lo",  packCRGB(CRGB(255,160,  0))));
  colorHumMid  = unpackCRGB(p.getUInt("h_col_mid", packCRGB(CRGB(  0,200, 50))));
  colorHumHi   = unpackCRGB(p.getUInt("h_col_hi",  packCRGB(CRGB(  0,160,255))));
  batThreshLo  = p.getUChar("b_thr_lo", 30);
  batThreshHi  = p.getUChar("b_thr_hi", 60);
  colorBatLo   = unpackCRGB(p.getUInt("b_col_lo",  packCRGB(CRGB(220, 40,  0))));
  colorBatMid  = unpackCRGB(p.getUInt("b_col_mid", packCRGB(CRGB(220,180,  0))));
  colorBatHi   = unpackCRGB(p.getUInt("b_col_hi",  packCRGB(CRGB(  0,200, 50))));
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
  // Display colors
  p.putUInt("col_clock",  packCRGB(colorClock));
  p.putUInt("col_poc",    packCRGB(colorPocsag));
  p.putFloat("t_thr_lo",  tempThreshLo);
  p.putFloat("t_thr_hi",  tempThreshHi);
  p.putUInt("t_col_lo",   packCRGB(colorTempLo));
  p.putUInt("t_col_mid",  packCRGB(colorTempMid));
  p.putUInt("t_col_hi",   packCRGB(colorTempHi));
  p.putFloat("h_thr_lo",  humThreshLo);
  p.putFloat("h_thr_hi",  humThreshHi);
  p.putUInt("h_col_lo",   packCRGB(colorHumLo));
  p.putUInt("h_col_mid",  packCRGB(colorHumMid));
  p.putUInt("h_col_hi",   packCRGB(colorHumHi));
  p.putUChar("b_thr_lo",  batThreshLo);
  p.putUChar("b_thr_hi",  batThreshHi);
  p.putUInt("b_col_lo",   packCRGB(colorBatLo));
  p.putUInt("b_col_mid",  packCRGB(colorBatMid));
  p.putUInt("b_col_hi",   packCRGB(colorBatHi));
  p.end();
}
