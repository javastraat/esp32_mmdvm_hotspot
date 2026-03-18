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
#include <WiFi.h>
#include <PubSubClient.h>

// ── Config globals ─────────────────────────────────────────────────────────────
bool     mqttEnabled   = false;
char     mqttBroker[64]= "";
uint16_t mqttPort      = 1883;
char     mqttUser[32]  = "";
char     mqttPass[64]  = "";
bool     mqttDiscovery = true;
char     mqttPrefix[32]= "homeassistant";
char     mqttNodeId[32]= "ulanzi";

// ── Internal state ─────────────────────────────────────────────────────────────
static WiFiClient    _wc;
static PubSubClient  _mqtt(_wc);

static char          _mac[13]         = "";   // 12-char hex MAC, e.g. "A1B2C3D4E5F6"
static bool          _connected       = false;
static char          _statusMsg[64]   = "Disabled";
static unsigned long _lastReconnect   = 0;
static unsigned long _lastState       = 0;

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

// Build the shared "device" JSON fragment (no trailing comma)
static int _devBlock(char* buf, int len) {
  return snprintf(buf, len,
    "\"device\":{"
      "\"identifiers\":[\"%s_%s\"],"
      "\"name\":\"%s\","
      "\"model\":\"TC001 ESP-NOW\","
      "\"manufacturer\":\"Ulanzi\","
      "\"configuration_url\":\"http://%s/\""
    "}",
    mqttNodeId, _mac,
    bootName,
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
  if (sht31Available) {
    _discSensor("temperature", "Temperature", "temperature", "\xc2\xb0\x43");  // °C UTF-8
    _discSensor("humidity",    "Humidity",    "humidity",    "%");
  }
  _discSensor("battery_pct", "Battery",        "battery",        "%");
  _discSensor("battery_mv",  "Battery Voltage","voltage",        "mV");
  _discSensor("rssi",        "WiFi RSSI",       "signal_strength","dBm");
  _discSensor("uptime",      "Uptime",          "duration",       "s");
#if RECV_POCSAG
  _discSensor("pocsag_msg",   "POCSAG Message", "", "");
  _discSensor("pocsag_count", "POCSAG Count",   "", "");
#endif
  _discSwitch("auto_brightness", "Auto Brightness");
  _discSwitch("debug_log",       "Debug Logging");
  _discNumber("brightness",      "Brightness",  1, 255, 1);
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
  char tmp[16];
  snprintf(tmp, sizeof(tmp), "%d", batPct);  _pubStr("sensor", "battery_pct", tmp);
  snprintf(tmp, sizeof(tmp), "%d", batMv);   _pubStr("sensor", "battery_mv",  tmp);
  snprintf(tmp, sizeof(tmp), "%d", WiFi.RSSI()); _pubStr("sensor", "rssi",    tmp);
  snprintf(tmp, sizeof(tmp), "%lu", millis() / 1000); _pubStr("sensor", "uptime", tmp);
#if RECV_POCSAG
  _pubStr("sensor", "pocsag_msg", pocsagMsgLen > 0 ? pocsagMsg : "");
  snprintf(tmp, sizeof(tmp), "%lu", (unsigned long)rxTotalPocsag);
  _pubStr("sensor", "pocsag_count", tmp);
#endif
  _pubStr("switch", "auto_brightness", autoBrightnessEnabled ? "ON" : "OFF");
  _pubStr("switch", "debug_log",       debugLogEnabled       ? "ON" : "OFF");
  snprintf(tmp, sizeof(tmp), "%d", currentBrightness);
  _pubStr("number", "brightness", tmp);
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

  if (sub == "switch/auto_brightness/set") {
    autoBrightnessEnabled = (strcmp(val, "ON") == 0);
    if (!autoBrightnessEnabled) FastLED.setBrightness(currentBrightness);
    saveSettings();
    _pubStr("switch", "auto_brightness", autoBrightnessEnabled ? "ON" : "OFF");
    LOG("[MQTT] auto_brightness → %s\n", val);

  } else if (sub == "switch/debug_log/set") {
    debugLogEnabled = (strcmp(val, "ON") == 0);
    saveSettings();
    _pubStr("switch", "debug_log", debugLogEnabled ? "ON" : "OFF");
    LOG("[MQTT] debug_log → %s\n", val);

  } else if (sub == "number/brightness/set") {
    int v = atoi(val);
    if (v >= 1 && v <= 255) {
      currentBrightness = (uint8_t)v;
      if (!autoBrightnessEnabled) FastLED.setBrightness(currentBrightness);
      saveSettings();
      char buf[8]; snprintf(buf, sizeof(buf), "%d", v);
      _pubStr("number", "brightness", buf);
      LOG("[MQTT] brightness → %d\n", v);
    }

  } else if (sub == "button/reboot/command" && strcmp(val, "PRESS") == 0) {
    LOG("[MQTT] Reboot via HA\n");
    delay(300);
    ESP.restart();

  } else if (sub == "button/clear_rtc/command" && strcmp(val, "PRESS") == 0) {
    LOG("[MQTT] Clear RTC via HA\n");
    if (rtcAvailable) ds1307Stop();
    timeSynced   = false;
    pocsagSynced = false;
    delay(300);
    ESP.restart();
  }
}

// ── Connect ────────────────────────────────────────────────────────────────────

static bool _doConnect() {
  if (strlen(mqttBroker) == 0) {
    strncpy(_statusMsg, "No broker configured", sizeof(_statusMsg));
    return false;
  }

  _mqtt.setServer(mqttBroker, mqttPort);
  _mqtt.setCallback(_callback);
  _mqtt.setBufferSize(1024);
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
  snprintf(sub, sizeof(sub), "%s/switch/+/set",      mqttNodeId); _mqtt.subscribe(sub);
  snprintf(sub, sizeof(sub), "%s/number/+/set",      mqttNodeId); _mqtt.subscribe(sub);
  snprintf(sub, sizeof(sub), "%s/button/+/command",  mqttNodeId); _mqtt.subscribe(sub);

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
  if (!mqttEnabled || !_mqtt.connected()) return;
#if RECV_POCSAG
  _pubStr("sensor", "pocsag_msg", pocsagMsg);
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)rxTotalPocsag);
  _pubStr("sensor", "pocsag_count", buf);
#endif
}

void mqttRequestReconnect() {
  if (_mqtt.connected()) _mqtt.disconnect();
  _connected    = false;
  _lastReconnect = 0;  // reconnect on next task iteration
  LOG("[MQTT] Reconnect requested\n");
}

bool        mqttIsConnected()  { return _connected; }
const char* mqttGetStatus()    { return _statusMsg; }
