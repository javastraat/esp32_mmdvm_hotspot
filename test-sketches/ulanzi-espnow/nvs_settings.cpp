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
  // Device name + mDNS hostname
  // Debug logging
  debugLogEnabled = p.getBool("debug_log", false);
  // MQTT
  mqttEnabled   = p.getBool  ("mqtt_en",     false);
  mqttPort      = p.getUShort("mqtt_port",   1883);
  mqttDiscovery = p.getBool  ("mqtt_disc",   true);
  { String s = p.getString("mqtt_broker", ""); s.trim(); strncpy(mqttBroker, s.c_str(), 63); mqttBroker[63] = '\0'; }
  { String s = p.getString("mqtt_user",   ""); s.trim(); strncpy(mqttUser,   s.c_str(), 31); mqttUser[31]   = '\0'; }
  { String s = p.getString("mqtt_pass",   ""); s.trim(); strncpy(mqttPass,   s.c_str(), 63); mqttPass[63]   = '\0'; }
  { String s = p.getString("mqtt_prefix", "homeassistant"); s.trim(); strncpy(mqttPrefix, s.c_str(), 31); mqttPrefix[31] = '\0'; }
  { String s = p.getString("mqtt_node",   "ulanzi");        s.trim(); strncpy(mqttNodeId, s.c_str(), 31); mqttNodeId[31] = '\0'; }
  { String s = p.getString("mqtt_ha_name", ""); s.trim(); strncpy(mqttHaName, s.c_str(), 31); mqttHaName[31] = '\0'; }
  { String s = p.getString("boot_name", "ULANZI"); s.trim(); s.toUpperCase(); strncpy(bootName, s.c_str(), 8); bootName[8] = '\0'; }
  { String s = p.getString("mdns_name", MDNS_HOSTNAME); s.trim(); s.toLowerCase(); strncpy(mdnsName, s.c_str(), 31); mdnsName[31] = '\0'; }
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
  tempThreshLo = p.getFloat ("t_thr_lo", 16.0f);
  tempThreshHi = p.getFloat ("t_thr_hi", 20.0f);
  colorTempLo  = unpackCRGB(p.getUInt("t_col_lo",  packCRGB(CRGB(  0,160,255))));
  colorTempMid = unpackCRGB(p.getUInt("t_col_mid", packCRGB(CRGB(  0,200, 50))));
  colorTempHi  = unpackCRGB(p.getUInt("t_col_hi",  packCRGB(CRGB(255, 80,  0))));
  humThreshLo  = p.getFloat ("h_thr_lo", 30.0f);
  humThreshHi  = p.getFloat ("h_thr_hi", 50.0f);
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
  // Debug logging
  p.putBool("debug_log", debugLogEnabled);
  // MQTT
  p.putBool  ("mqtt_en",     mqttEnabled);
  p.putUShort("mqtt_port",   mqttPort);
  p.putBool  ("mqtt_disc",   mqttDiscovery);
  p.putString("mqtt_broker", mqttBroker);
  p.putString("mqtt_user",   mqttUser);
  p.putString("mqtt_pass",   mqttPass);
  p.putString("mqtt_prefix", mqttPrefix);
  p.putString("mqtt_node",   mqttNodeId);
  p.putString("mqtt_ha_name", mqttHaName);
  // Device name + mDNS hostname
  p.putString("boot_name", bootName);
  p.putString("mdns_name", mdnsName);
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
