# ESP32 RTOS MMDVM — Code Review
**Date:** 2026-02-28 · **Version:** `20260228_ESP32_BETA`

---

## Quick-Reference: Easy Fixes

> Small, low-risk changes. Pick any row, go to the linked file, apply the fix, tick the checkbox in the Issue Tracker below.

### 2–4 lines (same pattern each)

| ID | File | What to change |
|----|------|----------------|
| **H-1** | [system_modem.cpp:657](system_modem.cpp#L657) | Add `unsigned long waitStart = millis();` + `&& (millis() - waitStart < 30000)` to the DMR guard `while` loop |
| **H-6** | [mmdvm_pocsag.cpp:583](mmdvm_pocsag.cpp#L583) | Same timeout pattern on the `while (pocsagTxInProgress)` loop — 60-second deadline |
| **H-7** | [mmdvm_pocsag.cpp:553](mmdvm_pocsag.cpp#L553) | Replace `while(true) { if (!dmrTxActive) break; … }` with `while (dmrTxActive && (millis()-waitStart < 30000))` |
| **M-6** | [include/config.h:132](include/config.h#L132) | Change `const int WEB_SERVER_PORT` / `const int LED_PIN` etc. → `constexpr int` |
| **M-16** | [mmdvm_dapnet.cpp:243](mmdvm_dapnet.cpp#L243) | Apply `jsonStr()` escaping to server response fields embedded in MQTT JSON |
| **M-18** | [web_handlers_wifi.cpp:60](web_handlers_wifi.cpp#L60) | Apply `jsonStr()` escaping to `wifiSlotSsid`, `wifiSlotPass`, `wifiSlotLabel` before embedding in JSON response |
| **L-5** | [mmdvm_dstar.cpp](mmdvm_dstar.cpp), [mmdvm_ysf.cpp](mmdvm_ysf.cpp), [mmdvm_p25.cpp](mmdvm_p25.cpp), [mmdvm_nxdn.cpp](mmdvm_nxdn.cpp) | Add `vTaskSuspend(NULL);` at top of each stub task loop body |

### Skip for now — significant refactor required

`H-2` `H-3` `H-4` `H-5` `H-8` `H-9` `H-11` `M-1` `M-2` `M-3` `M-4` `M-5` `M-7` `M-8` `M-9` `M-19` `L-1` `L-2` `L-6` `L-7` `L-8` `L-11`

---

## Issue Tracker

> Issues are listed once. Each entry has location, description, and concrete fix.
> Tick the checkbox when resolved.

---

### 🔴 CRITICAL

---

- [ ] **C-4 · OTA URLs point to internal dev server**
  **File:** [include/config.h:325–334](include/config.h#L325)
  ```cpp
  #define OTA_VERSION_URL  "http://192.168.2.220:3000/..."   // ← only reachable on your LAN
  ```
  Any user who builds from source gets broken OTA. Swap the commented GitHub URLs back in as default; move the local URLs behind a `#ifdef DEV_BUILD`.

---

- [ ] **C-5 · Credentials committed in `config.h`**
  **File:** [include/config.h:29,54,135,152](include/config.h#L29)
  ```cpp
  #define WIFI_PASSWORD    "itoldyoualready"
  #define WIFI_AP_PASSWORD "mmdvm1234"
  #define WEB_PASSWORD     "pi-star"
  #define DMR_PASSWORD     "passw0rd"
  ```
  Replace with `""` or `"CHANGE_ME"`. Move real credentials to `include/secrets.h` (which already exists and is in `.gitignore`).

---

### 🟠 HIGH

---

- [ ] **H-1 · CW ID DMR guard — infinite wait, no timeout**
  **File:** [system_modem.cpp:657](system_modem.cpp#L657)
  ```cpp
  while (dmrTxActive || dmrRfRxActive)   // ← no exit condition
      vTaskDelay(500 / portTICK_PERIOD_MS);
  ```
  If either flag gets stuck, modemTask waits forever — CW ID never fires again.
  ```cpp
  unsigned long waitStart = millis();
  while ((dmrTxActive || dmrRfRxActive) && (millis() - waitStart < 30000))
      vTaskDelay(500 / portTICK_PERIOD_MS);
  if (dmrTxActive || dmrRfRxActive)
      addLogMessage("[MODEM Task] CW ID: DMR wait timed out, transmitting anyway");
  ```

---

- [ ] **H-2 · DMR TX circular buffer — no atomic ops**
  **File:** [mmdvm_dmr.cpp:118–127](mmdvm_dmr.cpp#L118)
  ```cpp
  static volatile int dmrTxHead = 0;
  static volatile int dmrTxTail = 0;
  ```
  `volatile` alone does not provide read-modify-write atomicity. On a multi-core system a concurrent write can corrupt both values. Replace with a FreeRTOS `QueueHandle_t` (thread-safe, provides backpressure, no hand-rolled ring buffer needed).

---

- [ ] **H-3 · `MMDVM_SERIAL` has no mutex**
  **Files:** [system_modem.cpp](system_modem.cpp), [mmdvm_pocsag.cpp](mmdvm_pocsag.cpp), [mmdvm_dmr.cpp](mmdvm_dmr.cpp)
  Three tasks share Serial2 with no mutex — only the `pocsagTxInProgress` flag gates access. If the DMR task is mid-read when the flag is set, it still consumes the bytes.
  Long-term fix: introduce `SemaphoreHandle_t mmdvmSerialMutex` and wrap all Serial2 read/write.

---

- [ ] **H-4 · No CSRF protection on web forms**
  **File:** [system_webserver.cpp](system_webserver.cpp) and all web handlers
  POST endpoints for reboot, factory reset, mode change, POCSAG send, CW ID test have no CSRF token. Anyone on the same network who tricks the user into clicking a link can trigger these actions.
  Generate a random token at boot, embed it as a hidden field in all forms, reject POSTs without a matching token.

---

- [ ] **H-5 · WireGuard private key stored plaintext in NVS**
  **File:** [esp32-rtos-mmdvm.ino:381](esp32-rtos-mmdvm.ino#L381)
  NVS flash on the ESP32 is unencrypted by default. Physical flash dump exposes the WireGuard private key. Enable NVS encryption in the ESP-IDF partition table if the board supports it, or at minimum document this limitation.

---

- [ ] **H-6 · POCSAG task — infinite wait on CW ID (no timeout)**
  **File:** [mmdvm_pocsag.cpp:583](mmdvm_pocsag.cpp#L583)
  ```cpp
  while (pocsagTxInProgress)            // ← no exit condition
      vTaskDelay(100 / portTICK_PERIOD_MS);
  ```
  If the CW ID or modemTask leaves `pocsagTxInProgress` stuck true (crash, hang), the POCSAG task waits forever — no more paging messages ever transmitted.
  ```cpp
  unsigned long waitStart = millis();
  while (pocsagTxInProgress && (millis() - waitStart < 60000))
      vTaskDelay(100 / portTICK_PERIOD_MS);
  if (pocsagTxInProgress)
      addLogMessage("[POCSAG] CW ID wait timed out — proceeding anyway");
  ```

---

- [ ] **H-7 · POCSAG task — infinite nested loop waiting for DMR TX**
  **File:** [mmdvm_pocsag.cpp:553–568](mmdvm_pocsag.cpp#L553)
  The DMR-TX guard inside `pocsagTask()` is a `while(true)` loop with no outer timeout. If `dmrTxActive` gets stuck, the POCSAG task is permanently blocked:
  ```cpp
  while (true) {
      if (!dmrTxActive) break;           // ← stuck if dmrTxActive never clears
      vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  ```
  Apply the same pattern as H-1 and H-6 — add a 30-second deadline:
  ```cpp
  unsigned long waitStart = millis();
  while (dmrTxActive && (millis() - waitStart < 30000))
      vTaskDelay(100 / portTICK_PERIOD_MS);
  ```

---

- [ ] **H-8 · `publishMqtt()` called from multiple tasks — PubSubClient not thread-safe**
  **Files:** [system_mqtt.cpp](system_mqtt.cpp), [mmdvm_dmr.cpp](mmdvm_dmr.cpp), [system_modem.cpp](system_modem.cpp), [mmdvm_pocsag.cpp](mmdvm_pocsag.cpp)
  `publishMqtt()` is called by dmrTask, modemTask, and pocsagTask concurrently. PubSubClient uses a single internal write buffer with no synchronisation — concurrent calls corrupt the TCP stream, leading to dropped publishes or broker disconnects.
  ```cpp
  // system_mqtt.cpp — at file scope:
  static SemaphoreHandle_t mqttMutex = xSemaphoreCreateMutex();

  bool publishMqtt(const char *topic, const char *payload, bool retain) {
      if (!mqttConnected) return false;
      if (xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(200)) != pdTRUE) return false;
      bool ok = mqttClient.publish(topic, payload, retain);
      xSemaphoreGive(mqttMutex);
      return ok;
  }
  ```

---

- [ ] **H-9 · DMR HTTP lookup — SSL certificate verification disabled**
  **File:** [mmdvm_dmr.cpp:825](mmdvm_dmr.cpp#L825)
  ```cpp
  client.setInsecure();   // disables certificate validation
  ```
  Any network path between the ESP32 and RadioID.net can serve a forged response. Use `client.setCACert(radioid_root_ca)` with the RadioID.net root certificate pinned at compile time.

---

- [x] **H-10 · `ethConnected` — missing `volatile`, read from 18+ tasks**
  **Files:** [system_eth.cpp:23](system_eth.cpp#L23), [system/system_eth.h:24](system/system_eth.h#L24)
  `ethConnected` is written by the Ethernet event handler (invoked from the WiFi/lwIP event task on Core 0) and read unguarded from at least 18 other compilation units. Without `volatile`, the compiler may cache the value in a register and readers on a different core never see updates — Ethernet-only users appear permanently offline to all subsystems.
  ```cpp
  // system_eth.cpp line 23:
  volatile bool ethConnected = false;
  // system/system_eth.h line 24:
  extern volatile bool ethConnected;
  ```

---

- [ ] **H-11 · OLED call/TX history arrays — written by one task, read by another with no mutex**
  **File:** [system_oled.cpp:64–100, 217–250](system_oled.cpp#L64)
  `dmrCallHistory[]` is written by `setDmrTxUserInfo()` called from dmrTask (Core 1) and read by `getDmrActivityJson()` called from web server routes (Core 0). The Arduino `String` members inside the structs are especially unsafe — a concurrent write while a web response is building the JSON string can cause a heap-use-after-free crash.
  ```cpp
  // At file scope in system_oled.cpp:
  static SemaphoreHandle_t oledHistoryMutex = xSemaphoreCreateMutex();

  // Wrap setDmrTxUserInfo(), getDmrActivityJson(),
  //      setPocsagTxInfo(), getPocsagTxHistoryJson()
  // with xSemaphoreTake/Give around the array accesses.
  ```

---

### 🟡 MEDIUM

---

- [ ] **M-1 · `saveSettings()` called unconditionally on every boot — NVS wear**
  **File:** [esp32-rtos-mmdvm.ino:888](esp32-rtos-mmdvm.ino#L888)
  ```cpp
  loadSettings();
  saveSettings();  // writes 80+ NVS keys every boot
  ```
  Use a schema version key — only write when the firmware version changes:
  ```cpp
  loadSettings();
  {
    Preferences mig;
    mig.begin("mmdvm", false);
    String schema = mig.getString("fw_schema", "");
    bool needsMigration = (schema != String(FIRMWARE_VERSION));
    if (needsMigration) mig.putString("fw_schema", FIRMWARE_VERSION);
    mig.end();
    if (needsMigration) {
      saveSettings();
      addLogMessage("[Settings] Schema migrated to " + String(FIRMWARE_VERSION));
    }
  }
  ```

---

- [ ] **M-2 · Frame parsing duplicated 3× in `setupMMDVM()`**
  **File:** [system_modem.cpp:255–531](system_modem.cpp#L255)
  The MMDVM version frame parser is copy-pasted for wakeup serial, main serial fallback, and post-init retry. Extract to: `bool readModemVersion(HardwareSerial &serial, unsigned long timeoutMs, String &version)` and call it from all three sites.

---

- [ ] **M-3 · `restoreModemAfterCwid()` and `restoreModemAfterPocsag()` are near-identical**
  **Files:** [system_modem.cpp:572](system_modem.cpp#L572), [mmdvm_pocsag.cpp](mmdvm_pocsag.cpp)
  Both functions flush UART, re-send frequency, wait ACK, re-send the full 23-byte config, wait ACK. Merge into one:
  ```cpp
  static void restoreModem(uint32_t rxFreq, uint32_t txFreq);
  ```

---

- [ ] **M-4 · JSON escaping — three different implementations**
  **Files:** [mmdvm_dmr.cpp:41](mmdvm_dmr.cpp#L41), [system_logger.cpp:82](system_logger.cpp#L82), [mmdvm_dapnet.cpp:68](mmdvm_dapnet.cpp#L68)
  `jsonStr()` in DMR is correct. The two `String.replace()` versions in logger and dapnet are incomplete (miss `\t`, `\0`, control chars < 0x20).
  Move `jsonStr()` to [web/include/utils.h](web/include/utils.h) and use it everywhere.

---

- [ ] **M-5 · MQTT topic strings persisted to NVS unnecessarily**
  **File:** [esp32-rtos-mmdvm.ino:loadSettings()](esp32-rtos-mmdvm.ino#L404)
  ~20 MQTT topic strings are saved/loaded from NVS. End users never change topic names. Remove them from NVS — keep as compile-time constants in `config.h` only.

---

- [ ] **M-6 · `const int` / `const String` defined in header — ODR violation**
  **File:** [include/config.h:132,186](include/config.h#L132)
  ```cpp
  const int WEB_SERVER_PORT = 80;   // non-inline, included in multiple TUs
  const int LED_PIN = 38;
  ```
  Change to `constexpr int` (C++11, safe in headers) or `#define`.

---

- [ ] **M-7 · POCSAG shadow queue — enqueue/display update ordering**
  **File:** [mmdvm_pocsag.cpp](mmdvm_pocsag.cpp) (queue management)
  The shadow display array is updated *after* `xQueueSend`. If the POCSAG task dequeues the item before the shadow is written, the UI shows a ghost entry.
  Fix: update shadow array *before* calling `xQueueSend`.

---

- [ ] **M-8 · `initDmrLookupTask()` undocumented and unconfigured**
  **File:** [esp32-rtos-mmdvm.ino:990](esp32-rtos-mmdvm.ino#L990), [mmdvm_dmr.cpp](mmdvm_dmr.cpp)
  A second DMR task for SQLite/HTTP lookups is spawned alongside `dmrTask` but has no dedicated stack/priority constants in `config.h`.
  Add `DMR_LOOKUP_TASK_STACK` / `DMR_LOOKUP_TASK_PRIORITY` to `config.h` and add the task to the header comment.

---

- [ ] **M-9 · No input validation on web settings**
  **Files:** [web_handlers_dmr_settings.cpp](web_handlers_dmr_settings.cpp), other handlers
  User-supplied values (DMR ID, frequency, color code, callsign) are saved to NVS and passed to the modem without range/format checks. Validate at the HTTP handler boundary before saving.

---

- [ ] **M-16 · DAPNET server response embedded in JSON without escaping**
  **File:** [mmdvm_dapnet.cpp:243](mmdvm_dapnet.cpp#L243)
  The DAPNET server sends free-form text (rubric names, message body) embedded directly into MQTT JSON payloads via string concatenation. If the server sends a `"` or `\`, the JSON is malformed. Apply `jsonStr()` escaping.

---

- [ ] **M-18 · WiFi slot API — SSID/password/label returned in JSON without escaping**
  **File:** [web_handlers_wifi.cpp:60–62, 101](web_handlers_wifi.cpp#L60)
  ```cpp
  json += "\"ssid\":\""     + wifiSlotSsid[slot] + "\",";
  json += "\"password\":\"" + wifiSlotPass[slot] + "\"";
  ```
  A stored SSID or password containing `"` or `\` produces malformed JSON. Apply `jsonStr()`-style escaping to all three fields.

---

- [ ] **M-19 · Config export (`generateConfigString()`) includes all credentials in plaintext**
  **File:** [web_handlers_config.cpp:168, 188, 231, 269, 302, 358](web_handlers_config.cpp#L168)
  Embeds every credential unredacted (WiFi passwords, WireGuard private key, DMR password, MQTT password, web admin password, DAPNET auth key, ArduinoOTA password). The SD card snapshot is unauthenticated — physical access to the card exposes all credentials.
  Add a `bool redactSecrets` parameter and mask sensitive values when writing to SD card.

---

### 🟢 LOW / NICE TO HAVE

---

- [ ] **L-1 · `String` heap fragmentation in high-frequency RTOS tasks**
  **Files:** [mmdvm_dmr.cpp](mmdvm_dmr.cpp), [mmdvm_pocsag.cpp](mmdvm_pocsag.cpp), MQTT publish calls
  Arduino `String` operator `+` allocates on the heap. In tasks that run every 10 ms (DMR) this creates heap fragmentation over time. Replace MQTT payload building with `snprintf` into a fixed stack buffer.

---

- [ ] **L-2 · PSRAM not utilised**
  **File:** all tasks
  No code uses `ps_malloc()` or `heap_caps_malloc(MALLOC_CAP_SPIRAM)`. If the board has PSRAM, move the DMR user cache and web page string buffers there.

---

- [ ] **L-5 · Stub tasks (DSTAR/YSF/P25/NXDN) — consume stack with no function**
  **Files:** [mmdvm_dstar.cpp](mmdvm_dstar.cpp), [mmdvm_ysf.cpp](mmdvm_ysf.cpp), [mmdvm_p25.cpp](mmdvm_p25.cpp), [mmdvm_nxdn.cpp](mmdvm_nxdn.cpp)
  If a user enables an unimplemented mode they get a running task consuming stack with no behaviour. Each stub task should call `vTaskSuspend(NULL)` immediately after logging a warning.

---

- [ ] **L-6 · Web handlers implemented in header files — slow incremental builds**
  **Files:** [system/web_handlers_*.h](system/)
  All handlers are `#include`d into [system_webserver.cpp](system_webserver.cpp), making it one massive translation unit. Move implementations to `.cpp` files; keep only declarations in `.h`.

---

- [ ] **L-7 · `loadSettings()` / `saveSettings()` — same 80 keys listed three times**
  **File:** [esp32-rtos-mmdvm.ino](esp32-rtos-mmdvm.ino)
  Load, first-boot-write, and save each duplicate every key name. Define NVS key names as `#define NVS_KEY_CALLSIGN "callsign"` constants so each name is written once.

---

- [ ] **L-8 · NVS single namespace may hit 126-entry limit**
  **File:** [esp32-rtos-mmdvm.ino:loadSettings()](esp32-rtos-mmdvm.ino#L290)
  All ~80 settings share namespace `"mmdvm"`. ESP32 NVS allows max 126 entries per namespace. Currently at ~80 entries (64% full). Split into `"mmdvm_net"`, `"mmdvm_hw"`, `"mmdvm_proto"` as the project grows.

---

- [ ] **L-11 · DMR user cache — O(N) linear scan on every received packet**
  **File:** [mmdvm_dmr.cpp:737–744](mmdvm_dmr.cpp#L737)
  `getCachedDmrUserInfo()` scans all cache entries linearly on every DMRD packet. Replace the array with `std::unordered_map<uint32_t, DmrUserInfo>` for O(1) lookups, or at minimum sort the cache and use binary search.

---

- [x] **L-15 · `ntpSynced` — missing `volatile`, written by ntpTask, read by oledTask**
  **Files:** [system_ntp.cpp:16](system_ntp.cpp#L16), [system/system_ntp.h:13](system/system_ntp.h#L13)
  ```cpp
  // system_ntp.cpp:
  volatile bool ntpSynced = false;
  // system/system_ntp.h:
  extern volatile bool ntpSynced;
  ```

---

## Architecture Overview (reference)

### Task Map

| Task | Core | Priority | Stack Free (observed) | Notes |
|------|------|----------|-----------------------|-------|
| MODEM Task | 1 | 5 | 37.9 KB | Owns MMDVM_SERIAL init + CW ID |
| DMR Task | 1 | 3 | 30.8 KB | Real-time protocol, reads MMDVM_SERIAL every 10 ms |
| Web Server | 0 | 2 | 26.2 KB | Near WiFi stack — correct |
| WiFi Task | 0 | 2 | 29.3 KB | |
| SD Card | 0 | 2 | 21.3 KB | |
| OLED Task | 0 | 1 | 16.2 KB | |
| NTP Task | 0 | 1 | 9.4 KB | |
| MQTT Task | 0 | 1 | 6.8 KB | |
| OTA Task | 0 | 1 | 7.9 KB | |
| LED Task | 0 | 1 | 4.2 KB | |
| DMR Database | 0 | 1 | 10.9 KB | SQLite/HTTP lookup — correct on Core 0 |
| POCSAG Task | 1 | 2 | — | Must be Core 1 (MMDVM_SERIAL) |
| DAPNET Task | 0 | 1 | — | Pure TCP I/O — correct on Core 0 ✓ |
| arduino_events | 1 | 19 | — | Internal — can briefly preempt MODEM/DMR |

**Core split is already well-optimised.** MODEM and DMR correctly isolated on Core 1. All network/IO tasks on Core 0 next to the WiFi stack.

### MMDVM Serial Protocol Summary

```
Frame: [0xE0 | total_len | command | data...]
ACK:   [0xE0, 0x03, 0x70]
NAK:   [0xE0, 0x04, 0x7F, reason]

Key commands:
  0x00 GET_VERSION    0x02 SET_CONFIG    0x03 SET_MODE
  0x04 SET_FREQ       0x0A SEND_CWID     0x50 POCSAG_DATA
  0x18/0x1A DMR_DATA  0x70 ACK          0x7F NAK
```

### POCSAG / CW ID Sequencing (confirmed working)

```
POCSAG TX:  flush → SET_MODE(IDLE) → SET_FREQ(pocsag) → SET_CONFIG(POCSAG) → POCSAG_DATA → wait 1.1s → restore
CW ID:      flush → SET_MODE(IDLE) → [retune if needed] → SEND_CWID → wait (len×600ms + 3s) → restore
Guard:      pocsagTxInProgress=true gates DMR UART reads for both operations
```

---

## Positive Highlights

- **`waitForModemAck()` frame parser** — proper MMDVM frame state machine, correctly handles interleaved status frames.
- **Logger mutex** — `xSemaphoreCreateMutex()` around the circular buffer is the right pattern.
- **DMR SHA-256 auth** — mbedTLS implementation is correct.
- **POCSAG BCH + parity** — matches MMDVMHost reference exactly.
- **`jsonStr()` in DMR** — proper per-character JSON escaping.
- **`pocsagTxInProgress` gate on DMR serial reads** — the root cause (DMR stealing ACKs) was correctly identified and fixed.
- **CW ID full-wait strategy** — waiting the calculated TX duration regardless of ACK timing is the correct solution.
- **6-slot WiFi with AP fallback** — thoughtful UX.
- **WireGuard VPN integration** — impressive on an ESP32.
- **Empty `loop()`** — RTOS architecture is properly used throughout.
- **LittleFS for snapshots + NVS for live settings** — good separation of concerns.
- **DMR lookup in separate task** — blocking I/O correctly isolated from real-time protocol loop.

---

*Codebase version: `20260228_ESP32_BETA` · Last reviewed: 2026-02-28*
