# ulanzi-espnow — ESP-NOW POCSAG/DMR Display for Ulanzi TC001

Receives POCSAG pager messages and DMR radio packets from an MMDVM hotspot over **ESP-NOW** (no WiFi pairing required) and displays them on the Ulanzi TC001's 32×8 LED matrix. Shows a live clock between messages, with optional temperature/humidity from a SHT31 sensor.

---

## How it works

The MMDVM hotspot sends packets wirelessly using **ESP-NOW** — a connectionless peer-to-peer protocol that works on the same 2.4 GHz channel as WiFi but without an access point. The Ulanzi TC001 receives these packets, parses them, and renders them on its LED matrix.

**POCSAG** messages appear in amber and scroll (or sit static) across the display. A special time-beacon RIC (224) lets the hotspot synchronise the clock. The display returns to the clock face automatically after each message.

**DMR** packets are counted and logged — raw Homebrew protocol frames are tracked but not rendered on the display (POCSAG is the display protocol).

---

## Hardware — Ulanzi TC001

| Component | Detail |
|---|---|
| MCU | ESP32-WROOM-32D, Xtensa LX6 240 MHz, 8 MB Flash |
| LED Matrix | 32×8 WS2812B, serpentine layout, GPIO32 |
| Buzzer | Passive piezo, GPIO15 (LEDC PWM) |
| LDR | GL5516 ambient light sensor, GPIO35 |
| Buttons | Left GPIO26, Middle GPIO27, Right GPIO14 (all active LOW) |
| Battery | 4400 mAh LiPo, voltage divider on GPIO34 |
| RTC | DS1307 (optional), I2C — SDA GPIO21, SCL GPIO22 |
| Temp/Humidity | SHT31 (optional), I2C 0x44 — same bus as RTC |

---

## Firmware Architecture

Two FreeRTOS tasks on separate cores eliminate display stutter caused by web requests:

| Core | Task | Contents |
|---|---|---|
| 0 | `webTask` (8 KB stack) | `WebServer.handleClient()` + `ArduinoOTA.handle()` |
| 0 | ESP-NOW callback `onReceive()` | Interrupt-driven (WiFi stack) — queues POCSAG packets |
| 1 | Arduino `loop()` | Display, sensors, buttons, POCSAG queue drain |

POCSAG packets are handed from Core 0 → Core 1 through a FreeRTOS queue (`pocsagRxQueue`, depth 4), avoiding any shared-memory race on the display state.

### Source files

| File | Lines | Contents |
|---|---|---|
| `ulanzi-espnow.ino` | ~265 | Includes, **all** globals, `webTaskFn`, `setup()`, `loop()` |
| `display.ino` | ~346 | Font tables (digits, alpha, symbols), drawing primitives, `loopDisplay`, `loopBrightness`, `loopAutoRotate` |
| `receiver.ino` | ~232 | `onReceive()`, `processPocsagPacket()`, `applyPocsagTime()`, `setupReceiver()` |
| `web.ino` | ~196 | `setupOTA()`, `setupWebServer()`, all HTTP handlers |
| `sensor.ino` | ~95 | DS1307 RTC (direct I2C), SHT31 (30 s poll) |
| `buzzer.ino` | ~55 | Non-blocking LEDC tone engine, `setupBuzzer()` |
| `buttons.ino` | ~48 | Debounced button handler |
| `settings.ino` | ~41 | NVS Preferences `loadSettings()` / `saveSettings()` |

All files form a **single compilation unit** — the Arduino build system concatenates them (primary file first, sub-files alphabetically). All global variables are declared in `ulanzi-espnow.ino` and accessed directly by every sub-file. No `extern` declarations needed.

---

## Display Modes

| Priority | Mode | Trigger | Color |
|---|---|---|---|
| 1 (highest) | **OTA progress** | OTA update in progress | Cyan bar on row 7 |
| 2 | **POCSAG message** | Packet received | Amber |
| 3 | **Clock** | Default / after message expires | White |
| 3 | **Temperature** | Button Right or auto-rotate | Orange |
| 3 | **Humidity** | Button Right or auto-rotate | Cyan-blue |
| — | **Scanner** | No time sync yet | Blue pulse |

### POCSAG message display

- **Short message** (fits in 32 px): shown static for `POCSAG_STATIC_MS` (15 s), then clock resumes.
- **Long message** (wider than 32 px): scrolls left at 50 ms/pixel, repeating `POCSAG_SCROLL_PASSES` (3) times.
- Buzzer beeps once on receive (configurable).
- Auto-rotate pauses while a POCSAG message is active; rotation timer resets when it clears.

### Clock

- Shows HH:MM:SS in a custom 3×5 pixel font, centered on the 32×8 matrix.
- Time is set by:
  1. DS1307 RTC at boot (if fitted and running).
  2. POCSAG time-beacon RIC 224 — hotspot broadcasts `YYYYMMDDHHMMSS<YYMMDDHHmmSS>` periodically.
- After a POCSAG sync the RTC is updated and `pocsag_synced` is set in the web API.
- Scanner animation plays until the first sync arrives.

### Auto-rotate

Cycles clock → temperature → humidity → clock on a configurable timer (1–60 s). Requires SHT31 sensor. Controllable from the web UI or via API.

---

## POCSAG RIC assignment

| RIC | Purpose |
|---|---|
| 224 | **Time beacon** — `YYYYMMDDHHMMSS<YYMMDDHHmmSS>`, sets system clock + RTC |
| 8 | **Callsign** — transmitter ID; trailing digits are stripped before display |
| 208, 200, 216, 4520, 4521 | Excluded — received and logged but never shown on the LED matrix |
| any other | Displayed on LED matrix (scrolling or static) |

Configure excluded RICs, the time beacon RIC, and the callsign RIC in `config.h`.

---

## Web Interface

Connect to the device's IP in a browser (port 80). Supports light and dark themes (persisted).

**`/`  — Dashboard**

| Card | Contents |
|---|---|
| Display Preview | Live 32×8 canvas mirror of the LED matrix (updates every 2 s) |
| Device | Hostname, IP, uptime, free heap |
| WiFi | SSID, channel, RSSI with signal-strength badge, MAC |
| Hardware | Fixed ESP32 / matrix spec |
| Battery & Sensors | Battery voltage + %, LDR raw, SHT31 temperature + humidity (if fitted) |
| Clock | Current time, sync source, current display mode |
| ESP-NOW | DMR and POCSAG packet counters |
| Last POCSAG | Table of last 10 messages (newest first) with RIC + text |
| Brightness | Auto/manual toggle + slider, saved to NVS |
| Buzzer | Enable/volume for boot chime, POCSAG beep, button click; play-test button |
| Display Rotation | Enable auto-cycle + speed slider (1–60 s) |
| System | Reboot button |

**`/live`  — Fullscreen live display** (250 ms refresh, 20×20 px per LED)

---

## REST API

| Method | Endpoint | Description |
|---|---|---|
| GET | `/api/status` | Full JSON status (all metrics, settings, counters, POCSAG log) |
| GET | `/api/leds` | 256-pixel RRGGBB hex string for live display rendering |
| POST | `/api/brightness` | `auto=0/1`, `level=1-255` |
| POST | `/api/buzzer` | `boot_en`, `boot_vol`, `pocsag_en`, `pocsag_vol`, `click_en`, `click_vol` |
| POST | `/api/buzzer/test` | `type=boot/pocsag/click`, `vol=1-255` |
| POST | `/api/rotate` | `enabled=0/1`, `interval=1-60` |
| POST | `/api/reboot` | Restarts the device |

Settings changed via API are persisted to NVS immediately (survive reboot).

---

## OTA Update

```
Host:     ulanzi-clock.local   (or by IP)
Port:     3232
Password: mmdvm   (configurable in config.h)
```

While flashing, the display shows `UPDATE` + a cyan progress bar on row 7. On success it shows `DONE`; on failure `ERR` (red). OTA is handled in `webTask` on Core 0 so the display continues running during the update.

---

## Setup

### 1. Sender (MMDVM hotspot)

Flash the hotspot firmware with `espnowSenderEnabled = true`. The sender broadcasts POCSAG and/or DMR packets to the Ulanzi's ESP-NOW MAC address.

Get the Ulanzi's MAC from the serial monitor at first boot:

```
[INFO] My MAC (use as RECEIVER_MAC in sender config.h):
       AA:BB:CC:DD:EE:FF
```

Paste that into the sender's `config.h` as `RECEIVER_MAC`.

### 2. Ulanzi (this sketch)

Edit `config.h`:

```cpp
// Protocol — enable what your hotspot sends
#define RECV_POCSAG  true
#define RECV_DMR     false

// Same WiFi network as the hotspot (ensures matching ESP-NOW channel)
#define WIFI_SSID      "YourSSID"
#define WIFI_PASSWORD  "YourPassword"

// OTA
#define OTA_HOSTNAME  "ulanzi-clock"
#define OTA_PASSWORD  "mmdvm"

// Time beacon — must match hotspot config
#define TIME_POCSAG_RIC  224

// Callsign RIC — trailing digits are stripped
#define CALLSIGN_RIC  8

// RICs never shown on display (time beacon, housekeeping RICs, etc.)
#define POCSAG_DISPLAY_EXCLUDED_RICS  { 224, 208, 200, 216, 4520, 4521 }
```

Flash with Arduino IDE or `arduino-cli`.

#### Required Arduino libraries

| Library | Author | Install name | Purpose |
|---|---|---|---|
| **ESP32 Arduino core** | Espressif | *(board manager)* | ESP32 board support, tested with 3.x |
| **FastLED** | Daniel Garcia | `FastLED` | WS2812B LED matrix driver |
| **AnimatedGIF** | Larry Bank | `AnimatedGIF` | Animated GIF icon playback on LED matrix |
| **TJpg_Decoder** | Bodmer | `TJpg_Decoder` | JPEG icon decode and render |
| **PNGdec** | Larry Bank | `PNGdec` | PNG detection and decode (used during icon download) |
| **JPEGENC** | Larry Bank | `JPEGENC` | Re-encode downloaded PNG icons as JPEG for display |
| **SHT31** | Rob Tillaart | `SHT31` | Temperature/humidity sensor |

All libraries are available through the Arduino Library Manager (**Sketch → Include Library → Manage Libraries**).

### 3. WiFi channel matching

ESP-NOW uses the same channel as WiFi. The Ulanzi connects to your router first (same SSID as the hotspot), which locks both devices to the same channel. If you leave `WIFI_SSID` empty, the Ulanzi uses whatever channel the hotspot's beacon is on — this may not match if the router assigns a different channel at each boot.

---

## Calibration

### LDR auto-brightness

Watch `ldr_raw` in `/api/status` or the web dashboard while adjusting lighting:

```cpp
#define LDR_ADC_DARK    1600   // ADC when sensor is covered (your darkest condition)
#define LDR_ADC_BRIGHT  4000   // ADC in your brightest normal lighting
#define LDR_MIN_BRIGHTNESS  5  // floor brightness in auto mode (0–255)
```

Brightness is EMA-smoothed (¼ weight per 2 s sample) to avoid flickering.

### Battery ADC

The TC001 uses a large voltage divider — raw ADC range is 510–660, not a simple 1:2 ratio.

```cpp
#define BAT_RAW_EMPTY  510   // ADC at fully depleted (~3.0 V)
#define BAT_RAW_FULL   660   // ADC at fully charged (~4.2 V)
```

Watch `battery_raw` in `/api/status` at full and empty charge to recalibrate.

---

## Buttons

| Button | Action |
|---|---|
| Left (GPIO26) | Reserved (plays click sound) |
| Middle (GPIO27) | Toggle auto-brightness on/off |
| Right (GPIO14) | Cycle display mode: clock → temperature → humidity (requires SHT31) |

Manual mode resets to clock after 10 s of inactivity. All buttons debounced at 50 ms.

---

## Known issues / notes

- `WiFi.setSleep(false)` is required — WiFi power-save CPU pauses interrupt the RMT peripheral used by WS2812B, causing colour glitches on every `FastLED.show()`.
- The `webTask` stack is sized at 8 KB because `/api/status` allocates `char json[2500]` + `char logBuf[1100]` on the stack (~3.7 KB of locals). Do not reduce below 6 KB.
- Wire/I2C is used by both the DS1307 RTC and SHT31. Both are probed and used only from Core 1 (`loop()`), so no mutex is needed.
- POCSAG display state (`pocsagMsg[]`, scroll vars) is written only from Core 1 (via `processPocsagPacket()` called in `loop()`). The ESP-NOW callback on Core 0 uses a FreeRTOS queue (`xQueueSendFromISR`) to avoid data races.
