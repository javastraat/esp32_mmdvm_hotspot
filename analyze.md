# ESP32 MMDVM Hotspot — Code Review & Architecture Analysis

> **Scope:** All source files in the project root except the `esp32_mmdvm_hotspot/`,
> `mmdvm-software/`, and `sharkrf/` subdirectories.
> Reviewed: March 2026 | ~13 400 lines across 38 `.cpp` files.

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Architecture Overview](#2-architecture-overview)
3. [Critical Issues](#3-critical-issues)
4. [Reliability & Safety](#4-reliability--safety)
5. [Code Quality & Maintainability](#5-code-quality--maintainability)
6. [Performance](#6-performance)
7. [Security](#7-security)
8. [Positive Patterns Worth Keeping](#8-positive-patterns-worth-keeping)
9. [Recommended Roadmap](#9-recommended-roadmap)

---

## 1. Executive Summary

The project is a capable, feature-rich MMDVM hotspot firmware with solid peripheral
coverage (DMR, POCSAG, SD card, OTA, OLED, WireGuard, MQTT, Telegram, NTP, Ethernet).
The FreeRTOS task model is well-suited to the ESP32 and the individual modules are
generally readable.

The three most impactful areas to address are:

| Priority | Issue | Impact |
|----------|-------|--------|
| HIGH | ~200 global `extern` variables shared across all modules | Maintainability, correctness |
| HIGH | Fake/simulated sensor data shipped in production | Incorrect UI data |
| HIGH | Duplicate bootloader entry sequence (likely causes flash failures) | Reliability |
| MEDIUM | Monolithic `loadSettings()` / `saveSettings()` (~430 combined lines) | Maintainability |
| MEDIUM | `vTaskDelete()` without graceful teardown | Reliability / crash risk |
| MEDIUM | JSON built by string concatenation everywhere | Correctness |
| LOW | Hardcoded default WiFi password in `config.h` | Security |

---

## 2. Architecture Overview

```
esp32_mmdvm_hotspot.ino          <- Main entry; defines ALL ~200 global runtime variables
|
+-- system/                      <- Infrastructure services
|   +-- system_wifi.cpp          WiFi connection manager (6-slot fallback + Soft AP)
|   +-- system_eth.cpp           Ethernet via LAN8720
|   +-- system_webserver.cpp     ESP32 WebServer + auth middleware
|   +-- system_modem.cpp         MMDVM modem serial driver (Core 1)
|   +-- system_sdcard.cpp        SD card task + mutex
|   +-- system_oled.cpp          SSD1306 OLED display task
|   +-- system_logger.cpp        Circular log buffer (50 x 128 chars)
|   +-- system_sensor.cpp        !! STUB -- returns random data !!
|   +-- system_firmware.cpp      Firmware version tracking
|   +-- system_ntp.cpp           NTP time sync
|   +-- service_mqtt.cpp         MQTT client
|   +-- service_telegram.cpp     Telegram bot
|   +-- service_wireguard.cpp    WireGuard VPN
|
+-- mmdvm/                       <- Protocol implementations
|   +-- mmdvm_dmr.cpp            DMR / BrandMeister (full implementation)
|   +-- mmdvm_pocsag.cpp         POCSAG paging (full implementation)
|   +-- mmdvm_dapnet.cpp         DAPNET paging
|   +-- mmdvm_dstar/ysf/p25/nxdn.cpp   Partial / stub implementations
|
+-- web/pages/*.h                <- Web UI pages (C++ string generation in .h files)
|
+-- web_handlers_*.cpp           <- HTTP route handlers (9 files)
```

**Task map (all `xTaskCreatePinnedToCore`):**

| Task | Core | Priority |
|------|------|----------|
| MODEM | 1 | High |
| DMR / POCSAG / D-STAR / YSF / P25 / NXDN | 0 | Medium-High |
| WiFi / Ethernet | 0 | High |
| WebServer | 0 | Medium |
| MQTT | 0 | Low-Medium |
| SD Card / OLED / Sensor / NTP / Telegram / WireGuard | 0 | Low |

---

## 3. Critical Issues

### 3.1 Fake Sensor Data in Production (`system_sensor.cpp`)

**File:** `system_sensor.cpp:46-47`

```cpp
reading = random(20, 30);
cpuUsage = (float)random(0, 100) / 100.0;
```

The "CPU usage" value shown in the web status UI and published to MQTT is a **random
number**. There is no real temperature sensor being read, and the `cpuUsage` global
written here is the same variable read by the web status API. Every user sees random
0-100% CPU and 20-30 degrees C "sensor" readings.

**Fix:** Replace with real ESP32 metrics:

```cpp
// Real CPU temperature (ESP32 internal sensor)
float tempC = temperatureRead();

// Real free-heap-based load proxy (or remove cpuUsage entirely)
float heapFrac = 1.0f - ((float)ESP.getFreeHeap() / (float)ESP.getHeapSize());
cpuUsage = heapFrac;

// For real per-task CPU load use uxTaskGetRunTimeStats() with
// configGENERATE_RUN_TIME_STATS=1 in sdkconfig.
```

---

### 3.2 Duplicate Bootloader Entry Sequence (`modem_flasher.cpp`)

**File:** `modem_flasher.cpp:325-342`

The RESET + serial-init sequence is copy-pasted verbatim twice back-to-back inside
`modemEnterBootloader()`. The second copy is dead code that re-initialises the serial
port unnecessarily. More dangerously, the duplicate `MMDVM_SERIAL.begin()` call can
confuse the ESP32 UART driver and has almost certainly caused unexplained modem flash
failures.

The following block appears twice starting at line 325 and again at line 337:

```cpp
digitalWrite(MMDVM_RESET_PIN, HIGH);
delay(1000);
MMDVM_SERIAL.begin(MMDVM_SERIAL_BAUD, SERIAL_8E1, MMDVM_RX_PIN, MMDVM_TX_PIN);
delay(200);
```

**Fix:** Remove the duplicate block at lines 337-343. Keep only the first instance.

---

### 3.3 Global `extern` Variable Coupling

**Files:** All `mmdvm_*.cpp`, `system_*.cpp`, `service_*.cpp`, `web_handlers_*.cpp`

Every module declares its dependencies via naked `extern` statements. There are over
**200 global variables** defined in `esp32_mmdvm_hotspot.ino`, accessed by `extern`
in every other file. For example, `mmdvm_dmr.cpp` alone has 18 file-scope externs.

Consequences:
- Renaming or retyping any variable requires finding and updating every `extern`
- No compile-time check that a module is accessing the right namespace
- `saveSettings()` and `loadSettings()` must manually list every key (~430 lines total)
- Any task can mutate any setting at any time without synchronisation

**Recommended fix (incremental):** Group related settings into structs and pass
pointers or references:

```cpp
// include/settings.h
struct DmrSettings {
    String server;
    uint16_t port;
    String password;
    uint32_t rxFreq, txFreq;
    uint8_t colorCode, rfPower;
};

extern DmrSettings dmrSettings;  // one extern instead of six
```

Migrate one group at a time without requiring a full rewrite.

---

## 4. Reliability & Safety

### 4.1 `vTaskDelete()` Without Graceful Teardown

**File:** `web_handlers_admin.cpp:122-145, 219-250`

The `/api/restart-mmdvm` and `/api/restart-services` handlers call `vTaskDelete()` on
running protocol tasks without first signalling them to stop. A task deleted mid-flight
may hold a semaphore, have a UDP socket in an inconsistent state, or be writing to the
SD card. This can cause hard-to-reproduce crashes on the next restart.

**Fix:** Add a per-task stop flag (`volatile bool dmrStopRequested`), signal it, wait
up to 500 ms for the task to exit cleanly, then call `vTaskDelete()` only if it has
not already self-deleted via `vTaskDelete(NULL)`.

```cpp
dmrStopRequested = true;
unsigned long t = millis();
while (dmrTaskHandle != NULL && millis() - t < 500)
    vTaskDelay(10 / portTICK_PERIOD_MS);
if (dmrTaskHandle != NULL) { vTaskDelete(dmrTaskHandle); dmrTaskHandle = NULL; }
```

---

### 4.2 `millis()` Overflow in Timeout Loops

**File:** `modem_flasher.cpp:71`, `modem_flasher.cpp:114`

```cpp
unsigned long timeout = millis() + 2000;
while (millis() < timeout)  // WRONG: wraps around after ~49 days
```

The correct idiom is `while (millis() - start < 2000)` using unsigned subtraction
which wraps safely. All other modules use the correct idiom. Only `modem_flasher.cpp`
uses the broken pattern.

**Fix:**
```cpp
unsigned long start = millis();
while (millis() - start < 2000)
```

---

### 4.3 SD Card Mutex Held During Full File Transfer

**Files:** `sdcard_handlers.cpp` (browse download), `web_handlers_snapshots.cpp`

`server.streamFile()` blocks until the entire HTTP transfer completes while
`sdCardMutex` is held. During a large file download:
- The SD card monitoring task cannot run
- SQLite lookups (DMR ID queries) time out waiting for the mutex
- Log messages that need SD access are silently dropped

**Fix:** For small config files, copy to a memory buffer before releasing the mutex,
then stream from the buffer. For large database files, stream in chunks with brief
mutex releases between chunks.

---

### 4.4 Log Messages Silently Dropped Under Contention

**File:** `system_logger.cpp`

```cpp
if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) != pdTRUE)
    return;  // silently drops the message
```

Under heavy load (flash operations, DMR call bursts) log messages from multiple tasks
are silently discarded. The developer has no way to know how many messages were lost.

**Fix:** Add a dropped-message counter:

```cpp
static uint32_t logDropped = 0;
if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    logDropped++;
    return;
}
```

Expose `logDropped` in `/api/status` and `/api/logs`.

---

### 4.5 MQTT Self-Message Filter Is Fragile

**File:** `service_mqtt.cpp:89`

```cpp
if (message.indexOf("\"info\":") >= 0)
    return;  // skip own announce
```

Any inbound command from an external system that legitimately contains `"info":` in
any JSON field is silently ignored. This is a content-based filter on a structural
property.

**Fix:** Filter by topic instead. Subscribe to a dedicated command topic (e.g.
`hotspot/commands`) and publish status/announces to a separate status topic, making
content-based filtering unnecessary.

---

### 4.6 WireGuard Task Hangs Indefinitely Waiting for NTP

**File:** `service_wireguard.cpp:74-84`

If NTP never syncs (DNS failure, blocked UDP 123), the WireGuard task waits forever.
The warning counter resets every 60 iterations so warning messages repeat indefinitely.

**Fix:** Add a maximum retry limit and suspend the task after it is exceeded:

```cpp
if (timeWaitCount >= 180) {  // 3 minutes total
    addLogMessage("[WireGuard] Aborting: NTP unavailable after 3 minutes");
    vTaskSuspend(NULL);
    return;
}
```

---

## 5. Code Quality & Maintainability

### 5.1 Monolithic `loadSettings()` and `saveSettings()` (~430 Lines)

**File:** `esp32_mmdvm_hotspot.ino`

Both functions are ~215 lines each, manually listing every NVS key. Adding a new
setting requires touching both functions. There is no compile-time safety net if you
add a key in `loadSettings()` but forget `saveSettings()`.

Note: `web_handlers_config.cpp` already uses the ESP-IDF NVS iterator to enumerate
all keys automatically. That same approach can partially replace the manual load/save.
At a minimum, group the 80 keys into named sections with helper functions:

```cpp
void loadNetworkSettings(Preferences &prefs);
void loadDmrSettings(Preferences &prefs);
void loadMqttSettings(Preferences &prefs);
```

---

### 5.2 Web Page HTML Generated by C++ String Concatenation in Header Files

**Directory:** `web/pages/*.h`

All 26 web pages are generated by C++ string concatenation inside `.h` files. Two
structural problems:

1. **Duplicate symbol risk:** The headers define functions (not just declarations), so
   they can only be `#include`d in one `.cpp` file each. The comment in
   `web_handlers_admin.cpp` warns: _"the page headers define functions in the header
   body, so including them in multiple .cpp files causes duplicate symbol errors"_.

2. **Unmaintainable HTML:** Reading, diffing, or editing HTML embedded in C++ strings
   is cumbersome, and page generation consumes significant heap at runtime.

**Better approach:** Store HTML templates in LittleFS as `.html` files. The web server
streams them directly without any runtime string building:

```cpp
server.on("/", HTTP_GET, []() {
    File f = LittleFS.open("/index.html", "r");
    server.streamFile(f, "text/html");
    f.close();
});
```

Dynamic values can be injected with a simple `{{token}}` replacement during streaming.

---

### 5.3 JSON Built by String Concatenation Throughout Codebase

**Files:** `web_handlers_admin.cpp`, `service_mqtt.cpp`, `mmdvm_dmr.cpp`, and more

```cpp
json += "{\"id\":\"wifi\",\"label\":\"WiFi\",\"enabled\":true,\"connected\":" +
        String(wifiConn ? "true" : "false") + "},";
```

If any string value contains a double-quote, backslash, or control character the JSON
is malformed. The `jsonStr()` helper in `mmdvm_dmr.cpp:42-53` already does proper
escaping but is not used everywhere.

**Fix:** Use **ArduinoJson** for all JSON output. It handles escaping automatically:

```cpp
StaticJsonDocument<512> doc;
doc["id"] = "wifi";
doc["label"] = "WiFi";
doc["connected"] = wifiConn;
String out;
serializeJson(doc, out);
```

---

### 5.4 Repeated Task-Stop Code Block (DRY Violation)

**File:** `web_handlers_admin.cpp`

The 6-task deletion sequence appears verbatim in both `/api/restart-mmdvm` and
`/api/restart-services`. Likewise the 6-task restart sequence is repeated.

**Fix:** Extract a helper:

```cpp
static void stopAllModeTasks() {
    TaskHandle_t *handles[] = {
        &dmrTaskHandle, &dstarTaskHandle, &ysfTaskHandle,
        &p25TaskHandle, &nxdnTaskHandle, &pocsagTaskHandle };
    for (auto *h : handles) {
        if (*h != NULL) { vTaskDelete(*h); *h = NULL; }
    }
}
```

---

### 5.5 `extern` Declarations Inside Function Bodies

**Files:** `system_wifi.cpp:69-71`, `modem_flasher.cpp:260-268`

```cpp
void startSoftAP() {
    extern String wifiApSsid;      // inside function body
    extern String wifiApPassword;
```

Legal C++ but unusual and misleading. It obscures the file-level coupling.

**Fix:** Move all `extern` declarations to file scope, after the includes.

---

### 5.6 D-STAR / YSF / P25 / NXDN Have No Connection State

**Files:** `mmdvm_dstar.cpp`, `mmdvm_ysf.cpp`, `mmdvm_p25.cpp`, `mmdvm_nxdn.cpp`

The `/api/mode-status` endpoint correctly reports `"connected"` for DMR but omits it
for other modes. Users enabling D-STAR have no visibility into whether it is actually
connected. If these modes are functional, add `volatile bool dstarLoggedIn = false;`
etc. and expose in the API. If they are stubs, document that in the UI.

---

### 5.7 Config Import Has No Key Whitelist

**File:** `web_handlers_config.cpp:132-186`

`applyConfigString()` writes any `key:type=value` line to NVS without validating that
the key is a known setting. A malformed or malicious config file could write unexpected
keys or fill the NVS partition.

**Fix:** Validate keys against a known-keys set before writing. The `loadSettings()`
function already contains the canonical key list and can serve as a reference.

---

## 6. Performance

### 6.1 `String` Heap Fragmentation

The ESP32 Arduino `String` class uses heap allocation. The firmware makes heavy use of
`String` concatenation in hot paths (every MQTT publish, every log message, every HTTP
response). On a device with 320 KB SRAM and 16+ concurrent tasks, repeated small heap
allocations lead to fragmentation over long uptimes.

Good mitigations already exist in some places (`reserve()`, `snprintf` into fixed
buffers). Apply them consistently in hot paths. Monitor `ESP.getMinFreeHeap()` over
long uptimes -- a steadily shrinking minimum indicates fragmentation.

---

### 6.2 `publishHardwareInfo()` Builds a Large String via `+=`

**File:** `service_mqtt.cpp:325-416`

This function builds a ~600-byte JSON string via `+=` over 25+ fields. It is called
both on connect and periodically. Using `StaticJsonDocument<1024>` would reduce
allocations and guarantee correct escaping.

---

### 6.3 Sensor Task Logs Random Data Every 2 Seconds

**File:** `system_sensor.cpp`

Even ignoring the fake data, logging every 2 seconds adds 720 log entries/hour and
720 MQTT publishes/hour for meaningless values. Increase the interval to 30-60 seconds
as a minimum, and fix the data source.

---

## 7. Security

### 7.1 Hardcoded Default WiFi Password in `config.h`

**File:** `include/config.h`

```cpp
#define WIFI_PASSWORD "itoldyoualready"
```

This default password is visible in the source repository. Any device flashed without
changing defaults will connect to a network using this known password.

**Fix:** Set the default to an empty string. Force first-boot setup through the Soft AP
fallback mode before the hotspot attempts to connect to any network.

---

### 7.2 No CSRF Protection on Destructive Endpoints

**File:** `web_handlers_admin.cpp`

`/api/factory-reset`, `/api/reboot`, `/api/restart-mmdvm` are plain HTTP POST endpoints
protected only by Basic Auth. A malicious web page in the same browser session can
trigger these via CSRF since the browser sends credentials automatically.

**Fix:** Require a confirmation parameter for destructive endpoints:

```cpp
if (server.arg("confirm") != "yes") {
    server.send(400, "text/plain", "Missing confirmation");
    return;
}
```

---

### 7.3 MQTT Credentials Transmitted in Plaintext

**File:** `service_mqtt.cpp:35`

```cpp
WiFiClient wifiClient;  // no TLS
```

MQTT credentials are sent in plaintext. For deployments using internet-facing brokers
(WireGuard feature implies external connectivity), switch to `WiFiClientSecure` with
certificate verification.

---

## 8. Positive Patterns Worth Keeping

These areas are well-implemented and serve as reference patterns for the rest of the
codebase.

| Pattern | Location | Why it is good |
|---------|----------|----------------|
| NVS iterator config export | `web_handlers_config.cpp:33-125` | Zero-maintenance, picks up new keys automatically |
| MMDVM frame parser with re-sync | `system_modem.cpp:143-200` | Correctly skips stale frames before ACK; documents the old bug |
| Mutex + short timeout for SD monitoring | `system_sdcard.cpp:112` | Non-blocking skip rather than indefinite wait |
| MQTT client ID with chip MAC suffix | `service_mqtt.cpp:177-180` | Unique per device, survives hostname collisions |
| LittleFS path sanitization | `web_handlers_snapshots.cpp` | Blocks `..` traversal, enforces `/` prefix |
| MMDVM keepalive wakeup serial | `system_modem.cpp:229-231` | Correctly keeps modem alive via dedicated UART |
| `jsonStr()` helper for escaping | `mmdvm_dmr.cpp:42-53` | Proper JSON string escaping (needs wider adoption) |
| 6-slot WiFi with Soft AP fallback | `system_wifi.cpp` | Clean state machine with Ethernet coexistence |
| Config export/import shared helpers | `web_handlers_config.cpp` | HTTP routes and snapshot handlers reuse the same logic |

---

## 9. Recommended Roadmap

### Immediate (bug fixes, low effort)

1. **Remove duplicate bootloader sequence** in `modem_flasher.cpp:337-343` — likely
   the root cause of intermittent flash failures.
2. **Fix fake sensor data** in `system_sensor.cpp` — replace `random()` with
   `temperatureRead()` and real heap metrics.
3. **Fix `millis()` overflow** in `modem_flasher.cpp:71,114` — use elapsed-time idiom.
4. **Remove duplicate task-stop code** in `web_handlers_admin.cpp` — extract
   `stopAllModeTasks()` helper.
5. **Move inline `extern` declarations** to file scope in `system_wifi.cpp` and
   `modem_flasher.cpp`.

### Short-term (reliability, medium effort)

6. **Add graceful task shutdown** before `vTaskDelete()` in admin restart handlers.
7. **Add dropped-message counter** to the logger, expose in status API.
8. **Fix MQTT self-message filter** — filter by topic, not content substring.
9. **Add maximum NTP wait** in WireGuard task to prevent infinite hang.
10. **Add config import key whitelist** in `applyConfigString()`.

### Medium-term (architecture, larger effort)

11. **Adopt ArduinoJson** for all JSON output — eliminate string concatenation and
    escaping bugs.
12. **Group settings into structs** — reduce the 200-global-extern coupling
    incrementally, starting with DMR and MQTT settings.
13. **Split `loadSettings()` / `saveSettings()`** into per-module helpers.
14. **Add CSRF confirmation parameter** to destructive API endpoints.
15. **Add stack high-water mark monitoring** — call `uxTaskGetStackHighWaterMark()`
    in the sensor task, log results, alert when any task is within 512 bytes of its limit.

### Long-term (scalability)

16. **Move web UI to LittleFS HTML files** — decouple UI from firmware, eliminate
    duplicate-symbol risk in web page headers, enable live editing.
17. **Implement connection state** for D-STAR / YSF / P25 / NXDN modes.
18. **Enable TLS for MQTT** when connecting to internet-facing brokers.
19. **Change default WiFi password** to empty and enforce first-boot credential setup.

---

*End of review.*
