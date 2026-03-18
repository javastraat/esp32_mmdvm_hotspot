// sensor.cpp — DS1307 RTC (direct I2C) and SHT31 temperature/humidity sensor.
#include "sensor.h"
#include "globals.h"
#include <Wire.h>

// ── DS1307 RTC ────────────────────────────────────────────────────────────────

static uint8_t bcd2dec(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }
static uint8_t dec2bcd(uint8_t d) { return ((d / 10) << 4) | (d % 10); }

bool ds1307Read(struct tm& t) {
  Wire.beginTransmission(0x68);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) return false;
  Wire.requestFrom((uint8_t)0x68, (uint8_t)7);
  if (Wire.available() < 7) return false;
  uint8_t sec = Wire.read();
  if (sec & 0x80) return false;          // CH bit set = oscillator stopped
  uint8_t min = Wire.read();
  uint8_t hr  = Wire.read();
  Wire.read();                            // day-of-week (unused)
  uint8_t day = Wire.read();
  uint8_t mon = Wire.read();
  uint8_t yr  = Wire.read();
  t           = {};
  t.tm_sec    = bcd2dec(sec & 0x7F);
  t.tm_min    = bcd2dec(min);
  t.tm_hour   = bcd2dec(hr  & 0x3F);
  t.tm_mday   = bcd2dec(day);
  t.tm_mon    = bcd2dec(mon & 0x1F) - 1;
  t.tm_year   = bcd2dec(yr) + 100;       // 2-digit year → years since 1900
  t.tm_isdst  = -1;
  return true;
}

// Stop the DS1307 oscillator by setting the CH (clock halt) bit.
// ds1307Read() checks this bit and returns false, so setupRTC() will not
// restore the time on next boot — but rtcAvailable stays true so that
// applyPocsagTime() can write back to the RTC when a time beacon arrives.
void ds1307Stop() {
  Wire.beginTransmission(0x68);
  Wire.write(0x00);
  Wire.write(0x80);  // CH bit set → oscillator stopped
  Wire.endTransmission();
}

void ds1307Write(const struct tm& t) {
  Wire.beginTransmission(0x68);
  Wire.write(0x00);
  Wire.write(dec2bcd(t.tm_sec));          // CH=0 → oscillator running
  Wire.write(dec2bcd(t.tm_min));
  Wire.write(dec2bcd(t.tm_hour));
  Wire.write(1);                           // day-of-week (unused, set to 1)
  Wire.write(dec2bcd(t.tm_mday));
  Wire.write(dec2bcd(t.tm_mon + 1));
  Wire.write(dec2bcd(t.tm_year % 100));   // 2-digit year
  Wire.endTransmission();
}

void setupRTC() {
  Wire.begin(RTC_SDA_PIN, RTC_SCL_PIN);
  Wire.beginTransmission(0x68);
  if (Wire.endTransmission() != 0) {
    LOG("[RTC] Not found — waiting for POCSAG time beacon\n");
    return;
  }
  rtcAvailable = true;
  struct tm t = {};
  if (!ds1307Read(t)) {
    LOG("[RTC] Found but not running (lost power?) — waiting for POCSAG sync\n");
    return;
  }
  time_t epoch = mktime(&t);
  struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
  settimeofday(&tv, nullptr);
  timeSynced = true;
  LOG("[RTC] Time restored: %04d-%02d-%02d %02d:%02d:%02d\n",
    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
    t.tm_hour, t.tm_min, t.tm_sec);
}

// ── SHT31 ─────────────────────────────────────────────────────────────────────
// Wire already started by setupRTC(). Uses SHT31 library (Rob Tillaart).

void setupSHT31() {
  Wire.beginTransmission(0x44);
  if (Wire.endTransmission() != 0) {
    LOG("[SHT31] Not found\n");
    return;
  }
  sht31Available = true;
  sht31Sensor.read();
  sht31Temp = sht31Sensor.getHumidity();      // sensor returns T/RH swapped vs library labels
  sht31Hum  = sht31Sensor.getTemperature();
  LOG("[SHT31] Found — %.1fC  %.1f%%\n", sht31Temp, sht31Hum);
}

void loopSHT31() {
  if (!sht31Available) return;
  static unsigned long last = 0;
  if (millis() - last < 30000) return;
  last = millis();
  sht31Sensor.read();
  sht31Temp = sht31Sensor.getHumidity();      // sensor returns T/RH swapped vs library labels
  sht31Hum  = sht31Sensor.getTemperature();
  DLOG("[SHT31] %.1fC  %.1f%%\n", sht31Temp, sht31Hum);
}
