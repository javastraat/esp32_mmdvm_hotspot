// mqtt.cpp — MQTT connection + Home Assistant auto-discovery.
//
// Publishes all sensor values + state every 30 s and immediately on POCSAG.
// Subscribes to command topics so HA can control the device.
//
// Requires PubSubClient library (by Nick O'Leary, install via Library Manager).
#include "mqtt.h"
#include "globals.h"
#include "sensor.h"
#include "nvs_settings.h"
#include "display.h"
#include "buttons.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <LittleFS.h>

// ── Config globals ─────────────────────────────────────────────────────────────
bool     mqttEnabled   = false;
char     mqttBroker[64]= "";
uint16_t mqttPort      = 1883;
char     mqttUser[32]  = "";
char     mqttPass[64]  = "";
bool     mqttDiscovery = true;
char     mqttPrefix[32]= "homeassistant";
char     mqttNodeId[32]= "ulanzi";
char     mqttHaName[32]= "ulanzi";

// ── Internal state ─────────────────────────────────────────────────────────────
static WiFiClient    _wc;
static PubSubClient  _mqtt(_wc);

static char          _mac[13]         = "";   // 12-char hex MAC, e.g. "A1B2C3D4E5F6"
static bool          _connected       = false;
static char          _statusMsg[64]   = "Disabled";
static unsigned long _lastReconnect   = 0;
static unsigned long _lastState       = 0;
static volatile bool _statePending    = false;  // set by mqttNotifyState(), drained by task
static volatile bool _pocsagPending   = false;  // set by mqttNotifyPocsag(), drained by task

// ── Topic helpers ──────────────────────────────────────────────────────────────
// state:   {nodeId}/{component}/{id}/state
// command: {nodeId}/{component}/{id}/set   (button: /command)
// avail:   {nodeId}/availability
// disc:    {prefix}/{component}/{nodeId}_{mac}_{id}/config

static void _stTopic (char* b, int n, const char* comp, const char* id)
  { snprintf(b, n, "%s/%s/%s/state",   mqttNodeId, comp, id); }
static void _cmdTopic(char* b, int n, const char* comp, const char* id)
  { snprintf(b, n, "%s/%s/%s/set",     mqttNodeId, comp, id); }
static void _btnTopic(char* b, int n, const char* id)
  { snprintf(b, n, "%s/button/%s/command", mqttNodeId, id); }
static void _availTopic(char* b, int n)
  { snprintf(b, n, "%s/availability",  mqttNodeId); }

// Map brightness value to preset name
static const char* _brightnessPreset(uint8_t br) {
  if (br == 0)   return "Off";
  if (br == 10)  return "Night";
  if (br == 50)  return "Dim";
  if (br == 120) return "Medium";
  if (br == 255) return "Bright";
  return "Custom";
}

// Map screensaver brightness int16_t to preset name
static const char* _ssBrightnessPreset(int16_t br) {
  if (br == -2)  return "Auto";
  if (br == 0)   return "Off";
  if (br == 10)  return "Night";
  if (br == 50)  return "Dim";
  if (br == 120) return "Medium";
  if (br == 255) return "Bright";
  return "Dim";
}

// Build HA select options JSON for screensaver files (scans LittleFS /screensaver/)
static void _buildSsFileOptions(char* buf, int len) {
  int n = 0;
  n += snprintf(buf + n, len - n, "[\"None\"");
  File dir = LittleFS.open("/screensaver");
  if (dir && dir.isDirectory()) {
    File f = dir.openNextFile();
    while (f && n < len - 4) {
      if (!f.isDirectory()) {
        const char* nm = f.name();
        int nmlen = strlen(nm);
        if (nmlen > 4 && strcasecmp(nm + nmlen - 4, ".gif") == 0) {
          // build full path
          char path[72];
          snprintf(path, sizeof(path), "/screensaver/%s", nm);
          // escape path for JSON (/ is safe, no special chars expected)
          n += snprintf(buf + n, len - n, ",\"%s\"", path);
        }
      }
      f.close();
      f = dir.openNextFile();
    }
    dir.close();
  }
  if (n < len - 2) { buf[n++] = ']'; buf[n] = '\0'; }
}

// Build the shared "device" JSON fragment (no trailing comma)
// mqttNodeId  → topics + unique IDs (e.g. ulanzi/sensor/temperature/state)
// mqttHaName  → HA device name + entity_id prefix (e.g. sensor.ulanzi_temperature)
//               falls back to mqttNodeId if left empty
static int _devBlock(char* buf, int len) {
  const char* devName = (mqttHaName[0] != '\0') ? mqttHaName : mqttNodeId;
  return snprintf(buf, len,
    "\"device\":{"
      "\"identifiers\":[\"%s_%s\"],"
      "\"name\":\"%s\","
      "\"model\":\"TC001 ESP-NOW\","
      "\"hw_version\":\"TC001\","
      "\"sw_version\":\"%s\","
      "\"manufacturer\":\"PD2EMC\","
      "\"connections\":[[\"mac\",\"%.2s:%.2s:%.2s:%.2s:%.2s:%.2s\"]],"
      "\"configuration_url\":\"http://%s/\""
    "}",
    mqttNodeId, _mac,
    devName,
    ESP.getSdkVersion(),
    _mac, _mac+2, _mac+4, _mac+6, _mac+8, _mac+10,
    WiFi.localIP().toString().c_str());
}

// Publish a retained discovery config payload
static bool _pubDisc(const char* comp, const char* id, const char* json) {
  char topic[128];
  snprintf(topic, sizeof(topic), "%s/%s/%s_%s_%s/config",
    mqttPrefix, comp, mqttNodeId, _mac, id);
  return _mqtt.publish(topic, json, true);
}

// ── Discovery builders ─────────────────────────────────────────────────────────

static void _discSensor(const char* id, const char* name,
                         const char* dc, const char* unit) {
  char st[96], av[80], uid[72], dev[320], buf[920];
  _stTopic(st, sizeof(st), "sensor", id);
  _availTopic(av, sizeof(av));
  snprintf(uid, sizeof(uid), "%s_%s_%s", mqttNodeId, _mac, id);
  _devBlock(dev, sizeof(dev));

  int n = snprintf(buf, sizeof(buf),
    "{\"name\":\"%s\",\"unique_id\":\"%s\","
    "\"state_topic\":\"%s\","
    "\"availability_topic\":\"%s\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\"",
    name, uid, st, av);
  if (dc[0])   n += snprintf(buf+n, sizeof(buf)-n, ",\"device_class\":\"%s\"", dc);
  if (unit[0]) n += snprintf(buf+n, sizeof(buf)-n, ",\"unit_of_measurement\":\"%s\"", unit);
  n += snprintf(buf+n, sizeof(buf)-n, ",%s}", dev);
  _pubDisc("sensor", id, buf);
}

static void _discSwitch(const char* id, const char* name) {
  char st[96], cmd[96], av[80], uid[72], dev[320], buf[920];
  _stTopic(st, sizeof(st), "switch", id);
  _cmdTopic(cmd, sizeof(cmd), "switch", id);
  _availTopic(av, sizeof(av));
  snprintf(uid, sizeof(uid), "%s_%s_%s", mqttNodeId, _mac, id);
  _devBlock(dev, sizeof(dev));

  snprintf(buf, sizeof(buf),
    "{\"name\":\"%s\",\"unique_id\":\"%s\","
    "\"state_topic\":\"%s\","
    "\"command_topic\":\"%s\","
    "\"availability_topic\":\"%s\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\","
    "\"payload_on\":\"ON\",\"payload_off\":\"OFF\",%s}",
    name, uid, st, cmd, av, dev);
  _pubDisc("switch", id, buf);
}

static void _discNumber(const char* id, const char* name, int mn, int mx, int step) {
  char st[96], cmd[96], av[80], uid[72], dev[320], buf[920];
  _stTopic(st, sizeof(st), "number", id);
  _cmdTopic(cmd, sizeof(cmd), "number", id);
  _availTopic(av, sizeof(av));
  snprintf(uid, sizeof(uid), "%s_%s_%s", mqttNodeId, _mac, id);
  _devBlock(dev, sizeof(dev));

  snprintf(buf, sizeof(buf),
    "{\"name\":\"%s\",\"unique_id\":\"%s\","
    "\"state_topic\":\"%s\","
    "\"command_topic\":\"%s\","
    "\"availability_topic\":\"%s\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\","
    "\"min\":%d,\"max\":%d,\"step\":%d,%s}",
    name, uid, st, cmd, av, mn, mx, step, dev);
  _pubDisc("number", id, buf);
}

static void _discSelect(const char* id, const char* name, const char* optionsJson) {
  char st[96], cmd[96], av[80], uid[72], dev[320], buf[1024];
  _stTopic(st, sizeof(st), "select", id);
  _cmdTopic(cmd, sizeof(cmd), "select", id);
  _availTopic(av, sizeof(av));
  snprintf(uid, sizeof(uid), "%s_%s_%s", mqttNodeId, _mac, id);
  _devBlock(dev, sizeof(dev));

  snprintf(buf, sizeof(buf),
    "{\"name\":\"%s\",\"unique_id\":\"%s\","
    "\"state_topic\":\"%s\","
    "\"command_topic\":\"%s\","
    "\"availability_topic\":\"%s\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\","
    "\"options\":%s,%s}",
    name, uid, st, cmd, av, optionsJson, dev);
  _pubDisc("select", id, buf);
}

static void _discButton(const char* id, const char* name, const char* dc) {
  char cmd[96], av[80], uid[72], dev[320], buf[920];
  _btnTopic(cmd, sizeof(cmd), id);
  _availTopic(av, sizeof(av));
  snprintf(uid, sizeof(uid), "%s_%s_%s", mqttNodeId, _mac, id);
  _devBlock(dev, sizeof(dev));

  int n = snprintf(buf, sizeof(buf),
    "{\"name\":\"%s\",\"unique_id\":\"%s\","
    "\"command_topic\":\"%s\","
    "\"availability_topic\":\"%s\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\","
    "\"payload_press\":\"PRESS\"",
    name, uid, cmd, av);
  if (dc[0]) n += snprintf(buf+n, sizeof(buf)-n, ",\"device_class\":\"%s\"", dc);
  n += snprintf(buf+n, sizeof(buf)-n, ",%s}", dev);
  _pubDisc("button", id, buf);
}

static void _publishAllDiscovery() {
  if (!mqttDiscovery) return;

  // ── Sensors ────────────────────────────────────────────────────────────────
  if (sht31Available) {
    _discSensor("temperature", "Temperature", "temperature", "\xc2\xb0\x43");  // °C UTF-8
    _discSensor("humidity",    "Humidity",    "humidity",    "%");
  }
  _discSensor("battery_pct", "Battery",         "battery",        "%");
  _discSensor("battery_mv",  "Battery Voltage", "voltage",        "mV");
  _discSensor("rssi",        "WiFi RSSI",        "signal_strength","dBm");
  _discSensor("uptime",      "Uptime",           "duration",       "s");
  _discSensor("ldr_raw",     "LDR Raw",          "",               "");
#if RECV_POCSAG
  _discSensor("pocsag_msg",   "POCSAG Message", "", "");
  _discSensor("pocsag_count", "POCSAG Count",   "", "");
#endif
#if RECV_DMR
  _discSensor("dmr_count",    "DMR Count",      "", "");
#endif

  // ── Brightness ─────────────────────────────────────────────────────────────
  _discSwitch("auto_brightness", "Brightness Auto");
  _discNumber("brightness",      "Brightness",  1, 255, 1);
  _discSelect("brightness_preset", "Brightness Preset",
    "[\"Off\",\"Night\",\"Dim\",\"Medium\",\"Bright\",\"Custom\"]");

  // ── Buzzer ─────────────────────────────────────────────────────────────────
  _discSwitch("buzzer_boot",   "Buzzer Boot Sound");
  _discNumber("buzzer_boot_vol",   "Buzzer Boot Volume",   1, 255, 1);
  _discSwitch("buzzer_pocsag", "Buzzer POCSAG");
  _discNumber("buzzer_pocsag_vol", "Buzzer POCSAG Volume", 1, 255, 1);
  _discSwitch("buzzer_click",  "Buzzer Button Click");
  _discNumber("buzzer_click_vol",  "Buzzer Click Volume",  1, 255, 1);

  // ── Debug log ──────────────────────────────────────────────────────────────
  _discSwitch("debug_log", "Debug Log");

  // ── Display rotation ───────────────────────────────────────────────────────
  _discSwitch("auto_rotate",          "Rotate Screens");
  _discNumber("auto_rotate_interval", "Rotate Interval", 1, 60, 1);

  // ── Screensaver ────────────────────────────────────────────────────────────
  _discSwitch("screensaver",            "Screensaver");
  _discNumber("screensaver_timeout",    "Screensaver Timeout", 10, 3600, 10);
  _discSelect("screensaver_brightness", "Screensaver Brightness",
    "[\"Auto\",\"Off\",\"Night\",\"Dim\",\"Medium\",\"Bright\"]");
  { char opts[512]; _buildSsFileOptions(opts, sizeof(opts));
    _discSelect("screensaver_file", "Screensaver File", opts); }

  // ── Indicators ─────────────────────────────────────────────────────────────
  _discSwitch("indicators", "Indicators");

  // ── Display mode (read-only: shows active screen) ──────────────────────────
  _discSensor("display_mode", "Display Mode", "", "");

  // ── Physical button actions ────────────────────────────────────────────────
  _discButton("btn_left",   "Button Left",   "");
  _discButton("btn_middle", "Button Middle", "");
  _discButton("btn_right",  "Button Right",  "");

  // ── System actions ─────────────────────────────────────────────────────────
  _discButton("reboot",    "Reboot",    "restart");
  _discButton("clear_rtc", "Clear RTC", "");
}

// ── State publish ──────────────────────────────────────────────────────────────

static void _pubStr(const char* comp, const char* id, const char* val) {
  char topic[96];
  _stTopic(topic, sizeof(topic), comp, id);
  _mqtt.publish(topic, val);
}

static void _publishState() {
  char tmp[16];

  // Sensors
  if (sht31Available) {
    char tb[12], hb[12];
    dtostrf(sht31Temp, 0, 1, tb);
    dtostrf(sht31Hum,  0, 1, hb);
    _pubStr("sensor", "temperature", tb);
    _pubStr("sensor", "humidity",    hb);
  }
  int batRaw = analogRead(BAT_PIN);
  int batMv  = (int)map(constrain(batRaw, BAT_RAW_EMPTY, BAT_RAW_FULL),
                         BAT_RAW_EMPTY, BAT_RAW_FULL, BAT_EMPTY_MV, BAT_FULL_MV);
  int batPct = (int)constrain(map(batRaw, BAT_RAW_EMPTY, BAT_RAW_FULL, 0, 100), 0, 100);
  snprintf(tmp, sizeof(tmp), "%d", batPct);  _pubStr("sensor", "battery_pct", tmp);
  snprintf(tmp, sizeof(tmp), "%d", batMv);   _pubStr("sensor", "battery_mv",  tmp);
  snprintf(tmp, sizeof(tmp), "%d", WiFi.RSSI()); _pubStr("sensor", "rssi",    tmp);
  snprintf(tmp, sizeof(tmp), "%lu", millis() / 1000); _pubStr("sensor", "uptime", tmp);
  snprintf(tmp, sizeof(tmp), "%d", analogRead(LDR_PIN)); _pubStr("sensor", "ldr_raw", tmp);
#if RECV_POCSAG
  _pubStr("sensor", "pocsag_msg", pocsagMsgLen > 0 ? pocsagMsg : "");
  snprintf(tmp, sizeof(tmp), "%lu", (unsigned long)rxTotalPocsag);
  _pubStr("sensor", "pocsag_count", tmp);
#endif
#if RECV_DMR
  snprintf(tmp, sizeof(tmp), "%lu", (unsigned long)wsCountDmr);
  _pubStr("sensor", "dmr_count", tmp);
#endif

  // Debug log
  _pubStr("switch", "debug_log", debugLogEnabled ? "ON" : "OFF");

  // Brightness
  _pubStr("switch", "auto_brightness", autoBrightnessEnabled ? "ON" : "OFF");
  snprintf(tmp, sizeof(tmp), "%d", currentBrightness);
  _pubStr("number", "brightness", tmp);
  _pubStr("select", "brightness_preset", _brightnessPreset(currentBrightness));

  // Buzzer
  _pubStr("switch", "buzzer_boot",   buzzerBootEnabled   ? "ON" : "OFF");
  _pubStr("switch", "buzzer_pocsag", buzzerPocsagEnabled ? "ON" : "OFF");
  _pubStr("switch", "buzzer_click",  buzzerClickEnabled  ? "ON" : "OFF");
  snprintf(tmp, sizeof(tmp), "%d", buzzerBootVolume);   _pubStr("number", "buzzer_boot_vol",   tmp);
  snprintf(tmp, sizeof(tmp), "%d", buzzerPocsagVolume); _pubStr("number", "buzzer_pocsag_vol", tmp);
  snprintf(tmp, sizeof(tmp), "%d", buzzerClickVolume);  _pubStr("number", "buzzer_click_vol",  tmp);

  // Rotation
  _pubStr("switch", "auto_rotate", autoRotateEnabled ? "ON" : "OFF");
  snprintf(tmp, sizeof(tmp), "%d", autoRotateIntervalSec); _pubStr("number", "auto_rotate_interval", tmp);

  // Screensaver
  _pubStr("switch", "screensaver", screensaverEnabled ? "ON" : "OFF");
  snprintf(tmp, sizeof(tmp), "%d", screensaverTimeoutSec); _pubStr("number", "screensaver_timeout", tmp);
  _pubStr("select", "screensaver_brightness", _ssBrightnessPreset(screensaverBrightness));
  _pubStr("select", "screensaver_file", screensaverFile[0] ? screensaverFile : "None");

  // Indicators
  _pubStr("switch", "indicators", indicatorsEnabled ? "ON" : "OFF");

  // Display mode (sensor — read-only, reflects active screen)
  _pubStr("sensor", "display_mode", getScreenName());
}

// ── Incoming command callback ──────────────────────────────────────────────────

static void _callback(char* topic, byte* payload, unsigned int length) {
  char val[64] = {};
  int n = (length < sizeof(val) - 1) ? (int)length : (int)sizeof(val) - 1;
  memcpy(val, payload, n);
  val[n] = '\0';

  String t   = String(topic);
  String pfx = String(mqttNodeId) + "/";
  if (!t.startsWith(pfx)) return;
  String sub = t.substring(pfx.length());  // e.g. "switch/auto_brightness/set"

  // ── Brightness ──────────────────────────────────────────────────────────────
  if (sub == "switch/auto_brightness/set") {
    autoBrightnessEnabled = (strcmp(val, "ON") == 0);
    if (!autoBrightnessEnabled) FastLED.setBrightness(currentBrightness);
    saveSettings();
    _pubStr("switch", "auto_brightness", autoBrightnessEnabled ? "ON" : "OFF");
    _pubStr("select", "brightness_preset", _brightnessPreset(currentBrightness));
    LOG("[MQTT] auto_brightness → %s\n", val);

  } else if (sub == "number/brightness/set") {
    int v = atoi(val);
    if (v >= 1 && v <= 255) {
      currentBrightness = (uint8_t)v;
      if (!autoBrightnessEnabled) FastLED.setBrightness(currentBrightness);
      saveSettings();
      char buf[8]; snprintf(buf, sizeof(buf), "%d", v);
      _pubStr("number", "brightness", buf);
      _pubStr("select", "brightness_preset", _brightnessPreset((uint8_t)v));
      LOG("[MQTT] brightness → %d\n", v);
    }

  } else if (sub == "select/brightness_preset/set") {
    int newBr = -1;
    if      (strcmp(val, "Off")    == 0) newBr = 0;
    else if (strcmp(val, "Night")  == 0) newBr = 10;
    else if (strcmp(val, "Dim")    == 0) newBr = 50;
    else if (strcmp(val, "Medium") == 0) newBr = 120;
    else if (strcmp(val, "Bright") == 0) newBr = 255;
    if (newBr >= 0) {
      autoBrightnessEnabled = false;
      currentBrightness = (uint8_t)newBr;
      FastLED.setBrightness((uint8_t)newBr);
      saveSettings();
      _pubStr("switch", "auto_brightness", "OFF");
      if (newBr > 0) {
        char buf[8]; snprintf(buf, sizeof(buf), "%d", newBr);
        _pubStr("number", "brightness", buf);
      }
      _pubStr("select", "brightness_preset", val);
      LOG("[MQTT] brightness_preset → %s (%d)\n", val, newBr);
    }

  // ── Debug log ───────────────────────────────────────────────────────────────
  } else if (sub == "switch/debug_log/set") {
    debugLogEnabled = (strcmp(val, "ON") == 0);
    saveSettings();
    _pubStr("switch", "debug_log", debugLogEnabled ? "ON" : "OFF");
    LOG("[MQTT] debug_log → %s\n", val);

  // ── Buzzer switches ─────────────────────────────────────────────────────────
  } else if (sub == "switch/buzzer_boot/set") {
    buzzerBootEnabled = (strcmp(val, "ON") == 0);
    saveSettings();
    _pubStr("switch", "buzzer_boot", buzzerBootEnabled ? "ON" : "OFF");
    LOG("[MQTT] buzzer_boot → %s\n", val);

  } else if (sub == "switch/buzzer_pocsag/set") {
    buzzerPocsagEnabled = (strcmp(val, "ON") == 0);
    saveSettings();
    _pubStr("switch", "buzzer_pocsag", buzzerPocsagEnabled ? "ON" : "OFF");
    LOG("[MQTT] buzzer_pocsag → %s\n", val);

  } else if (sub == "switch/buzzer_click/set") {
    buzzerClickEnabled = (strcmp(val, "ON") == 0);
    saveSettings();
    _pubStr("switch", "buzzer_click", buzzerClickEnabled ? "ON" : "OFF");
    LOG("[MQTT] buzzer_click → %s\n", val);

  // ── Buzzer volumes ──────────────────────────────────────────────────────────
  } else if (sub == "number/buzzer_boot_vol/set") {
    int v = atoi(val);
    if (v >= 1 && v <= 255) {
      buzzerBootVolume = (uint8_t)v;
      saveSettings();
      char buf[8]; snprintf(buf, sizeof(buf), "%d", v);
      _pubStr("number", "buzzer_boot_vol", buf);
      LOG("[MQTT] buzzer_boot_vol → %d\n", v);
    }

  } else if (sub == "number/buzzer_pocsag_vol/set") {
    int v = atoi(val);
    if (v >= 1 && v <= 255) {
      buzzerPocsagVolume = (uint8_t)v;
      saveSettings();
      char buf[8]; snprintf(buf, sizeof(buf), "%d", v);
      _pubStr("number", "buzzer_pocsag_vol", buf);
      LOG("[MQTT] buzzer_pocsag_vol → %d\n", v);
    }

  } else if (sub == "number/buzzer_click_vol/set") {
    int v = atoi(val);
    if (v >= 1 && v <= 255) {
      buzzerClickVolume = (uint8_t)v;
      saveSettings();
      char buf[8]; snprintf(buf, sizeof(buf), "%d", v);
      _pubStr("number", "buzzer_click_vol", buf);
      LOG("[MQTT] buzzer_click_vol → %d\n", v);
    }

  // ── Auto-rotation ───────────────────────────────────────────────────────────
  } else if (sub == "switch/auto_rotate/set") {
    autoRotateEnabled = (strcmp(val, "ON") == 0);
    saveSettings();
    _pubStr("switch", "auto_rotate", autoRotateEnabled ? "ON" : "OFF");
    LOG("[MQTT] auto_rotate → %s\n", val);

  } else if (sub == "number/auto_rotate_interval/set") {
    int v = atoi(val);
    if (v >= 1 && v <= 60) {
      autoRotateIntervalSec = (uint8_t)v;
      saveSettings();
      char buf[8]; snprintf(buf, sizeof(buf), "%d", v);
      _pubStr("number", "auto_rotate_interval", buf);
      LOG("[MQTT] auto_rotate_interval → %d\n", v);
    }

  // ── Screensaver ─────────────────────────────────────────────────────────────
  } else if (sub == "switch/screensaver/set") {
    screensaverEnabled = (strcmp(val, "ON") == 0);
    resetScreensaverIdle();  // restores main brightness if ss was active; resets idle timer
    saveSettings();
    _pubStr("switch", "screensaver", screensaverEnabled ? "ON" : "OFF");
    LOG("[MQTT] screensaver → %s\n", val);

  } else if (sub == "number/screensaver_timeout/set") {
    int v = atoi(val);
    if (v >= 10 && v <= 3600) {
      screensaverTimeoutSec = (uint16_t)v;
      saveSettings();
      char buf[8]; snprintf(buf, sizeof(buf), "%d", v);
      _pubStr("number", "screensaver_timeout", buf);
      LOG("[MQTT] screensaver_timeout → %d\n", v);
    }

  } else if (sub == "select/screensaver_brightness/set") {
    int16_t newBr = screensaverBrightness;
    if      (strcmp(val, "Auto")   == 0) newBr = -2;
    else if (strcmp(val, "Off")    == 0) newBr = 0;
    else if (strcmp(val, "Night")  == 0) newBr = 10;
    else if (strcmp(val, "Dim")    == 0) newBr = 50;
    else if (strcmp(val, "Medium") == 0) newBr = 120;
    else if (strcmp(val, "Bright") == 0) newBr = 255;
    screensaverBrightness = newBr;
    screensaverApplyBrightness();  // take effect immediately if screensaver is playing
    saveSettings();
    _pubStr("select", "screensaver_brightness", val);
    LOG("[MQTT] screensaver_brightness → %s\n", val);

  } else if (sub == "select/screensaver_file/set") {
    if (strcmp(val, "None") == 0) {
      screensaverFile[0] = '\0';
    } else {
      strncpy(screensaverFile, val, 63); screensaverFile[63] = '\0';
    }
    _gifCloseIfOpen();  // reload on next screensaver frame
    saveSettings();
    _pubStr("select", "screensaver_file", screensaverFile[0] ? screensaverFile : "None");
    LOG("[MQTT] screensaver_file → %s\n", val);

  // ── Buttons ─────────────────────────────────────────────────────────────────
  // ── Indicators ──────────────────────────────────────────────────────────────
  } else if (sub == "switch/indicators/set") {
    indicatorsEnabled = (strcmp(val, "ON") == 0);
    saveSettings();
    _pubStr("switch", "indicators", indicatorsEnabled ? "ON" : "OFF");
    LOG("[MQTT] indicators → %s\n", val);

  } else if (sub == "button/btn_left/command"   && strcmp(val, "PRESS") == 0) {
    triggerButton(0);
    LOG("[MQTT] btn_left\n");

  } else if (sub == "button/btn_middle/command" && strcmp(val, "PRESS") == 0) {
    triggerButton(1);
    LOG("[MQTT] btn_middle\n");

  } else if (sub == "button/btn_right/command"  && strcmp(val, "PRESS") == 0) {
    triggerButton(2);
    LOG("[MQTT] btn_right\n");

  } else if (sub == "button/reboot/command" && strcmp(val, "PRESS") == 0) {
    LOG("[MQTT] Reboot via HA\n");
    delay(300);
    ESP.restart();

  } else if (sub == "button/clear_rtc/command" && strcmp(val, "PRESS") == 0) {
    LOG("[MQTT] Clear RTC via HA\n");
    if (rtcAvailable) ds1307Stop();
    timeSynced   = false;
    pocsagSynced = false;
    displayMode  = MODE_CLOCK;  // ensure scanner is visible immediately
  }

  // Guarantee state reaches HA even if the inline _pubStr above was dropped
  // by PubSubClient while its buffer was still processing the incoming message.
  mqttNotifyState();
}

// ── Connect ────────────────────────────────────────────────────────────────────

static bool _doConnect() {
  if (strlen(mqttBroker) == 0) {
    strncpy(_statusMsg, "No broker configured", sizeof(_statusMsg));
    return false;
  }

  _mqtt.setServer(mqttBroker, mqttPort);
  _mqtt.setCallback(_callback);
  _mqtt.setBufferSize(1280);
  _mqtt.setKeepAlive(60);

  char clientId[48];
  snprintf(clientId, sizeof(clientId), "%s_%s", mqttNodeId, _mac + 6);  // last 6 of MAC

  char avail[80];
  _availTopic(avail, sizeof(avail));

  bool ok = (strlen(mqttUser) > 0)
    ? _mqtt.connect(clientId, mqttUser, mqttPass, avail, 1, true, "offline")
    : _mqtt.connect(clientId, nullptr, nullptr,   avail, 1, true, "offline");

  if (!ok) {
    snprintf(_statusMsg, sizeof(_statusMsg), "Connect failed (rc=%d)", _mqtt.state());
    LOG("[MQTT] Connect to %s:%u FAILED, rc=%d\n", mqttBroker, mqttPort, _mqtt.state());
    return false;
  }

  LOG("[MQTT] Connected to %s:%u as %s\n", mqttBroker, mqttPort, clientId);
  snprintf(_statusMsg, sizeof(_statusMsg), "Connected to %s", mqttBroker);

  // Announce online (retained)
  _mqtt.publish(avail, "online", true);

  // Subscribe to command topics (wildcard per component)
  char sub[96];
  snprintf(sub, sizeof(sub), "%s/switch/+/set",     mqttNodeId); _mqtt.subscribe(sub);
  snprintf(sub, sizeof(sub), "%s/number/+/set",     mqttNodeId); _mqtt.subscribe(sub);
  snprintf(sub, sizeof(sub), "%s/select/+/set",     mqttNodeId); _mqtt.subscribe(sub);
  snprintf(sub, sizeof(sub), "%s/button/+/command", mqttNodeId); _mqtt.subscribe(sub);

  // Publish discovery + immediate state
  _publishAllDiscovery();
  _publishState();
  return true;
}

// ── FreeRTOS task (Core 0) ─────────────────────────────────────────────────────

static void mqttTaskFn(void*) {
  vTaskDelay(pdMS_TO_TICKS(6000));  // let WiFi + webserver fully settle first

  LOG("[MQTT] task started — %s\n",
    mqttEnabled ? "enabled" : "disabled (configure at /mqtt)");

  // Build MAC suffix once (WiFi must be up)
  while (WiFi.status() != WL_CONNECTED)
    vTaskDelay(pdMS_TO_TICKS(1000));
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  strncpy(_mac, mac.c_str(), 12);
  _mac[12] = '\0';

  for (;;) {
    if (!mqttEnabled || WiFi.status() != WL_CONNECTED) {
      if (_mqtt.connected()) {
        _mqtt.disconnect();
        _connected = false;
      }
      strncpy(_statusMsg,
        mqttEnabled ? "No WiFi" : "Disabled",
        sizeof(_statusMsg));
      vTaskDelay(pdMS_TO_TICKS(5000));
      continue;
    }

    if (!_mqtt.connected()) {
      _connected = false;
      unsigned long now = millis();
      if (now - _lastReconnect >= 10000) {
        _lastReconnect = now;
        _connected = _doConnect();
      }
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    _connected = true;
    _mqtt.loop();

    // Immediate POCSAG publish requested from core 1
    if (_pocsagPending) {
      _pocsagPending = false;
#if RECV_POCSAG
      _pubStr("sensor", "pocsag_msg", pocsagMsg);
      char pbuf[16];
      snprintf(pbuf, sizeof(pbuf), "%lu", (unsigned long)rxTotalPocsag);
      _pubStr("sensor", "pocsag_count", pbuf);
#endif
    }

    // Immediate state publish requested from web handlers
    if (_statePending) {
      _statePending = false;
      _lastState = millis();  // reset periodic timer so we don't double-publish
      _publishState();
    }

    // Periodic state publish every 30 s
    unsigned long now = millis();
    if (now - _lastState >= 30000) {
      _lastState = now;
      _publishState();
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ── Public API ─────────────────────────────────────────────────────────────────

void initMqttTask() {
  xTaskCreatePinnedToCore(mqttTaskFn, "mqttTask", 6144, nullptr, 1, nullptr, 0);
}

void mqttNotifyPocsag() {
  if (mqttEnabled) _pocsagPending = true;
}

void mqttNotifyState() {
  if (mqttEnabled) _statePending = true;
}

void mqttRequestReconnect() {
  if (_mqtt.connected()) _mqtt.disconnect();
  _connected    = false;
  _lastReconnect = 0;  // reconnect on next task iteration
  LOG("[MQTT] Reconnect requested\n");
}

bool        mqttIsConnected()  { return _connected; }
const char* mqttGetStatus()    { return _statusMsg; }
