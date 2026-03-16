// settings.ino — NVS Preferences: load and save all user-configurable settings.
// All globals (autoBrightnessEnabled, currentBrightness, buzzer*, autoRotate*)
// are declared in ulanzi-espnow.ino.

static void loadSettings() {
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
  p.end();
}

static void saveSettings() {
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
  p.end();
}
