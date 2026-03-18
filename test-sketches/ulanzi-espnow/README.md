# ulanzi-espnow — ESP-NOW POCSAG/DMR Display for Ulanzi TC001

Receives POCSAG pager messages and DMR radio packets from an MMDVM hotspot over **ESP-NOW**
and displays them on the Ulanzi TC001's 32×8 WS2812B LED matrix.
Shows a live clock between messages, with optional temperature / humidity / battery displays.
Fully configurable from a mobile-friendly web interface — no recompile needed.

---

## How it works

The MMDVM hotspot sends raw packets wirelessly over **ESP-NOW** — a connectionless
peer-to-peer 2.4 GHz protocol that works without an access point.
The Ulanzi TC001 receives these packets and renders them on its LED matrix.

- **POCSAG** messages display with a pinned icon and scrolling or static text.
  A special time-beacon RIC (224) lets the hotspot synchronise the display clock.
- **DMR** packets are counted and logged but not rendered on the display.
- Between messages the display cycles through clock, temperature, humidity, and battery.

---

## Hardware — Ulanzi TC001

| Component | Detail |
|---|---|
| MCU | ESP32-WROOM-32D · Xtensa LX6 240 MHz · 8 MB Flash |
| LED Matrix | 32×8 WS2812B · serpentine wiring · GPIO32 |
| Buzzer | Passive piezo · GPIO15 · LEDC PWM |
| LDR | GL5516 ambient light sensor · GPIO35 |
| Buttons | Left GPIO26 · Middle GPIO27 · Right GPIO14 (active LOW, pull-up) |
| Battery | 4400 mAh LiPo · voltage divider on GPIO34 |
| RTC | DS1307 (optional) · I2C · SDA GPIO21 · SCL GPIO22 |
| Temp/Humidity | SHT31 (optional) · I2C address 0x44 · same bus as RTC |

---

## Firmware Architecture

Two FreeRTOS tasks on separate cores keep the display smooth even during web requests:

| Core | Task | Responsibility |
|---|---|---|
| **0** | `webTask` (8 KB stack) | `WebServer.handleClient()` + `ArduinoOTA.handle()` |
| **0** | ESP-NOW `onReceive()` | Interrupt-driven — enqueues POCSAG packets via `xQueueSendFromISR` |
| **1** | Arduino `loop()` | Display, buttons, sensors, POCSAG queue drain |

POCSAG packets travel Core 0 → Core 1 through a FreeRTOS queue (`pocsagRxQueue`, depth 4).
All display state is written only from Core 1, eliminating data races without mutexes.

### Source files

| File | Purpose |
|---|---|
| `ulanzi-espnow.ino` | Global variable definitions · `setup()` · `loop()` |
| `config.h` | All compile-time constants and pin assignments |
| `globals.h` | Shared `extern` declarations and packet structs |
| `display.h/.cpp` | Font tables · drawing helpers · brightness · auto-rotate · display loop |
| `receiver.h/.cpp` | ESP-NOW callback · POCSAG processing · WiFi setup |
| `sensor.h/.cpp` | DS1307 RTC (direct I2C) · SHT31 temperature/humidity |
| `buzzer.h/.cpp` | Non-blocking LEDC tone engine |
| `buttons.h/.cpp` | Debounced button handler |
| `nvs_settings.h/.cpp` | NVS Preferences load/save |
| `filesystem.h/.cpp` | LittleFS initialisation |
| `web_server.h/.cpp` | ArduinoOTA + mDNS + WebServer routes |
| `web/styles.h` | Shared CSS + light/dark theme |
| `web/navigation.h` | Shared nav bar + LIVE modal |
| `web/main.h` | Home/dashboard page |
| `web/settings.h` | Settings page |
| `web/system.h` | System information page |
| `web/files.h` | File manager page |

---

## Display Modes

Priority order (highest wins):

| Priority | Mode | Trigger | Default color |
|---|---|---|---|
| 1 | **OTA progress** | OTA update started | Cyan bar on row 7 |
| 2 | **POCSAG message** | Packet received | Amber |
| 3 | **IP scroll** | WiFi connected at boot | Green |
| 4 | **Icon preview** | Show button in Settings | — |
| 5 | **Screensaver** | Idle timeout | GIF animation |
| 6 | **Clock** | Default | Configurable |
| 6 | **Temperature** | Button / auto-rotate | Dynamic zone color |
| 6 | **Humidity** | Button / auto-rotate | Dynamic zone color |
| 6 | **Battery** | Button / auto-rotate | Dynamic zone color |
| — | **Scanner** | No time sync yet | Blue pulse |

All steady-state colors (clock, POCSAG) and zone threshold colors (temp/hum/bat) are
configurable from the web Settings page and persisted to NVS.

### POCSAG message display

- **Static** (message fits on screen): displayed for `POCSAG_STATIC_MS` (default 15 s) then clock resumes.
  Icon animates at its natural GIF frame rate. Text is centered in the space to the right of the icon.
- **Scrolling** (message too wide): icon pinned at x=0, text scrolls left at `POCSAG_SCROLL_SPEED_MS`
  (default 50 ms/pixel), repeated `POCSAG_SCROLL_PASSES` (default 3) times.
  Text is clipped so it disappears behind the pinned icon rather than overlapping it.
- Buzzer beeps once on receive (configurable per-type: boot / POCSAG / click).
- Auto-rotate pauses while a message is active; rotation timer resets when it clears.

### Temperature / Humidity / Battery

- Each mode draws a configurable icon from LittleFS at x=0, followed by the value centered
  in the remaining space.
- If no icon file is set or the file is missing, the value is centered across the full 32 pixels.
- **Temperature** is displayed as `21.45°` — a custom degree symbol glyph (small circle) is
  rendered from the font table; the `C` suffix is omitted to save space.
- Colors are dynamic: three zones (Low / Mid / High) with independently configurable
  threshold values and colors, set from the Settings page.

### Clock

- Shows HH:MM:SS in a custom 3×5 pixel font, centered on the 32×8 matrix.
- Time source priority:
  1. DS1307 RTC at boot (if fitted and running).
  2. POCSAG time-beacon RIC 224 — the hotspot broadcasts `YYYYMMDDHHMMSS<YYMMDDHHmmSS>` periodically.
     After sync the RTC is updated and `pocsag_synced` is flagged in the API.
- Scanner animation (blue pulse) plays until the first sync arrives.

### Screensaver

- Activates after a configurable idle timeout (seconds).
- Plays a 32×8 GIF stored in `/screensaver/` on LittleFS.
- Any button press, incoming POCSAG message, or OTA event cancels the screensaver.
- File, timeout, and enable/disable are set from the Settings page.

### Auto-rotate

Cycles clock → temperature → humidity → battery → clock on a configurable timer (1–60 s).
Sensor modes are skipped if SHT31 is not detected.
Pauses during POCSAG messages and restores the previous mode when done.

### Boot screen

Displays the device name letter by letter in rainbow colours at startup.
The name (default `ULANZI`, max 8 characters) is configurable from the Settings page
**Device Name** card and persisted to NVS — no recompile needed.
`loadSettings()` runs before `drawBootScreen()` so the saved name is always shown.

---

## Network

### WiFi

The device connects to the configured `WIFI_SSID` at boot. This must be the same network
as the MMDVM hotspot so they share the same 2.4 GHz channel for ESP-NOW.

### mDNS

Once WiFi is connected the device advertises itself via mDNS:

```
http://<mdns-hostname>.local/
```

Default hostname is `ulanzi` → `http://ulanzi.local/`. The hostname is configurable
from the **Device Name** card in Settings (persisted to NVS, takes effect immediately).
The HTTP service is registered so network scanners can discover the device automatically.

### ArduinoOTA

OTA updates are available over WiFi. The OTA hostname (shown in the Arduino IDE
**Port** menu) is separately configurable from the **Device Name** card in Settings.
Default: `ulanzi-ota`. Change takes effect immediately without a reboot.

```
Host:     ulanzi-ota  (or device IP)
Port:     3232
Password: set OTA_PASSWORD in config.h (leave empty to disable)
```

During OTA: the matrix shows `UPDATE` + a cyan progress bar on row 7.
On success: `DONE` (green). On error: `ERR` (red) then returns to normal.
OTA runs in `webTask` on Core 0 — the display loop continues uninterrupted.

---

## POCSAG RIC reference

| RIC | Role |
|---|---|
| 224 | **Time beacon** — `YYYYMMDDHHMMSS<YYMMDDHHmmSS>` · sets system clock and DS1307 RTC |
| 8 | **Callsign** — transmitter ID; trailing digits are stripped before display |
| 208 · 200 · 216 · 4520 · 4521 | **Excluded** — logged but never shown on the matrix |
| *any other* | **Displayed** — static or scrolling depending on length |

Excluded RICs, time beacon RIC, and callsign RIC are all set in `config.h`.

---

## Web Interface

Connect to `http://<device-ip>/` or `http://<mdns-hostname>.local/` in any browser (port 80).
Light and dark themes are available (preference stored in `localStorage`).

### Pages

#### `/` — Dashboard

| Section | Contents |
|---|---|
| Live preview | 32×8 canvas mirror of the LED matrix · refreshes every 500 ms |
| Device | Hostname · IP · uptime · free heap |
| WiFi | SSID · channel · RSSI with signal-strength indicator · MAC |
| Battery & Sensors | Battery voltage + % · LDR raw · SHT31 temp + humidity (if fitted) |
| Clock | Current time · sync source (RTC / POCSAG / none) · display mode |
| ESP-NOW | DMR and POCSAG packet counters |
| POCSAG log | Last 10 messages (newest first) · RIC + text |

#### `/live` — Fullscreen Live Display

32×8 matrix rendered at 20×20 px per LED, refreshes every 250 ms.
Accessible from the LIVE button in the navigation bar.

#### `/settings` — Settings

| Card | Controls |
|---|---|
| **Brightness** | Auto (LDR) / manual toggle · slider 1–255 · presets: Night / Dim / Medium / Bright |
| **Buzzer** | Enable + volume (0–255) for Boot Sound, POCSAG Receive, Button Click · Test buttons |
| **Display Rotation** | Enable auto-cycle · interval slider 1–60 s |
| **Screensaver** | Enable · timeout (seconds) · GIF file selector · Test button |
| **Icons** | File picker for Temp / Humidity / Battery / POCSAG icons · Preview image · Show on matrix |
| **Text Colors** | Color picker for Clock text and POCSAG message text |
| **Thresholds & Colors** | Temperature: 2 thresholds + 3 zone colors · Humidity: same · Battery: same |
| **Device Name** | Boot screen name (max 8 chars) · mDNS hostname · ArduinoOTA hostname · Reboot button |

All settings are saved to NVS immediately on change — survive reboot.

#### `/system` — System Information

Chip model · revision · cores · CPU MHz · CPU temperature · heap · flash · sketch size ·
OTA free space · reset reason · SDK version · build date/time · webTask stack watermark.
Actions: **Clear RTC** (with confirmation) · **Reboot**.

#### `/files` — File Manager

Storage usage bar · LaMetric icon downloader (ID → HTTPS proxy → PNG → JPEG → save) ·
Full file browser: navigate directories · upload files · create folders · rename · delete · download.
Click a `.gif` or `.jpg` filename to preview inline.

---

## REST API

All endpoints respond with JSON unless noted. POST bodies use
`application/x-www-form-urlencoded`.
Settings POSTs write to NVS immediately.

### Status & display

| Method | Endpoint | Description |
|---|---|---|
| GET | `/api/status` | Full JSON — clock, time-sync flags, battery, LDR, SHT31, POCSAG log, brightness, buzzer, rotation, display mode, WiFi, free heap |
| GET | `/api/leds` | 256-pixel RRGGBB hex string (`RRGGBBRRGGBB…`) for live canvas rendering |

### Brightness

| Method | Endpoint | Body params | Description |
|---|---|---|---|
| POST | `/api/brightness` | `auto=0/1` · `level=1-255` | Set auto or manual brightness |

### Buzzer

| Method | Endpoint | Body params | Description |
|---|---|---|---|
| POST | `/api/buzzer` | `boot_en=0/1` · `boot_vol=0-255` · `pocsag_en=0/1` · `pocsag_vol=0-255` · `click_en=0/1` · `click_vol=0-255` | Save buzzer settings |
| POST | `/api/buzzer/test` | `type=boot/pocsag/click` · `vol=1-255` | Play a test tone immediately |

### Display rotation

| Method | Endpoint | Body params | Description |
|---|---|---|---|
| POST | `/api/rotate` | `enabled=0/1` · `interval=1-60` | Auto-rotate on/off and interval (seconds) |

### Icons

| Method | Endpoint | Body params | Description |
|---|---|---|---|
| GET | `/api/icons` | — | Current icon paths for temp / hum / bat / POCSAG |
| POST | `/api/icons` | `temp_icon` · `hum_icon` · `bat_icon` · `poc_icon` | Save icon file paths |
| POST | `/api/icons/preview` | `path=/icons/file.gif` | Show icon on matrix for 5 s |
| GET | `/api/icons/proxy?id=NNNNN` | — | Proxy LaMetric icon PNG over HTTPS (for browser canvas conversion) |

### Display colors

| Method | Endpoint | Body params | Description |
|---|---|---|---|
| GET | `/api/colors` | — | All current color and threshold values |
| POST | `/api/colors` | See table below | Update any combination of colors / thresholds |

POST `/api/colors` accepts any subset of:

| Param | Format | Meaning |
|---|---|---|
| `clock` | `#RRGGBB` | Clock text color |
| `poc` | `#RRGGBB` | POCSAG message text color |
| `t_thr_lo` · `t_thr_hi` | float (°C) | Temperature zone thresholds |
| `t_lo` · `t_mid` · `t_hi` | `#RRGGBB` | Temperature zone colors (below lo / between / above hi) |
| `h_thr_lo` · `h_thr_hi` | float (%) | Humidity zone thresholds |
| `h_lo` · `h_mid` · `h_hi` | `#RRGGBB` | Humidity zone colors |
| `b_thr_lo` · `b_thr_hi` | integer 0-100 (%) | Battery zone thresholds |
| `b_lo` · `b_mid` · `b_hi` | `#RRGGBB` | Battery zone colors |

### Screensaver

| Method | Endpoint | Body params | Description |
|---|---|---|---|
| GET | `/api/screensaver` | — | Current screensaver settings and active state |
| POST | `/api/screensaver` | `enabled=0/1` · `timeout=5-3600` · `file=/screensaver/x.gif` | Save screensaver settings |
| POST | `/api/screensaver/test` | `action=test/stop` | Start or stop screensaver immediately |

### Device identity

| Method | Endpoint | Body params | Description |
|---|---|---|---|
| GET | `/api/bootname` | — | Current boot screen name |
| POST | `/api/bootname` | `name=MYNAME` | Set boot screen name (1–8 chars, auto-uppercased) |
| GET | `/api/mdnsname` | — | Current mDNS hostname |
| POST | `/api/mdnsname` | `name=mydevice` | Set mDNS hostname (1–31 chars, a-z 0-9 -) · takes effect immediately |
| GET | `/api/otahostname` | — | Current ArduinoOTA hostname |
| POST | `/api/otahostname` | `name=mydevice-ota` | Set OTA hostname (1–31 chars, a-z 0-9 -) · takes effect immediately |

### Filesystem

| Method | Endpoint | Body / Query params | Description |
|---|---|---|---|
| GET | `/api/fs` | — | LittleFS total / used / available bytes |
| GET | `/api/fs/ls?path=/dir` | `path` | List directory entries (name, path, isDir, size) |
| GET | `/api/fs/download?path=/file` | `path` | Download file as attachment |
| POST | `/api/fs/delete` | `path=/file` | Delete file or empty directory |
| POST | `/api/fs/mkdir` | `path=/newdir` | Create directory |
| POST | `/api/fs/rename` | `from=/old` · `to=/new` | Rename / move file or directory |
| POST | `/api/files/upload?dir=/icons` | multipart form data | Upload file to specified directory |

### System

| Method | Endpoint | Body params | Description |
|---|---|---|---|
| GET | `/api/sysinfo` | — | Chip model, CPU MHz, temp, heap, flash, build info, reset reason, webTask stack |
| POST | `/api/rtc/clear` | — | Stop DS1307 oscillator, clear time-sync flags, reboot |
| POST | `/api/reboot` | — | Reboot immediately |

---

## Icons (LittleFS)

Icons are stored on the ESP32's LittleFS filesystem. Upload them via the **Files** page
or with `arduino-cli` / `esptool`.

| Directory | Contents |
|---|---|
| `/icons/` | GIF or JPEG icons for Temp / Humidity / Battery / POCSAG |
| `/screensaver/` | GIF files exactly **32×8 pixels** for the screensaver |

**Supported formats:** GIF (animated or static) and JPEG.
Icons should be 8×8 pixels for best results. The display centers them vertically.

**GIF looping:** both GIFs with the Netscape infinite-loop extension and plain
single-play GIFs loop continuously — when `playFrame` returns 0 (last frame) the
file handle is closed and reopened on the next tick, restarting from frame 0.

**LaMetric icons:** the Files page has a built-in downloader. Enter a LaMetric icon ID,
preview the image, and save it directly to `/icons/` — no PC needed.

---

## Setup

### 1. Sender (MMDVM hotspot)

Flash the hotspot firmware with ESP-NOW sender enabled.
Get the Ulanzi's MAC address from the serial monitor at first boot:

```
[INFO] My MAC: AA:BB:CC:DD:EE:FF
```

Paste it into the hotspot's sender config as `RECEIVER_MAC`.

### 2. Ulanzi — edit `config.h`

```cpp
// Enable the protocols your hotspot sends
#define RECV_POCSAG  true
#define RECV_DMR     false

// Same WiFi network as the hotspot (ensures matching ESP-NOW channel)
#define WIFI_SSID      "YourSSID"
#define WIFI_PASSWORD  "YourPassword"

// OTA password (leave empty to disable password protection)
#define OTA_PASSWORD  "ulanzi"

// Time beacon RIC (must match hotspot)
#define TIME_POCSAG_RIC  224

// Callsign RIC (trailing digits stripped before display)
#define CALLSIGN_RIC  8

// RICs that are never shown on the LED matrix
#define POCSAG_DISPLAY_EXCLUDED_RICS  { 224, 208, 200, 216, 4520, 4521 }
```

> **Note:** the boot screen name, mDNS hostname, and OTA hostname are all set at runtime
> from the **Device Name** card in Settings — no recompile needed.

### 3. Required Arduino libraries

Install via **Sketch → Include Library → Manage Libraries**:

| Library | Author | Purpose |
|---|---|---|
| ESP32 Arduino core ≥ 3.x | Espressif | Board support (install via Board Manager) |
| FastLED | Daniel Garcia | WS2812B LED matrix |
| AnimatedGIF | Larry Bank | Animated GIF playback |
| TJpg_Decoder | Bodmer | JPEG decode and render |
| SHT31 | Rob Tillaart | Temperature/humidity sensor |

### 4. Flash

Select board **ESP32 Dev Module** (or equivalent), 921600 baud, then Upload.
After first boot, subsequent updates can be done via OTA.

---

## Calibration

### LDR auto-brightness

Watch `ldr_raw` in the dashboard while adjusting room lighting:

```cpp
#define LDR_ADC_DARK    1600   // ADC when sensor is fully covered (darkest condition)
#define LDR_ADC_BRIGHT  4000   // ADC in your brightest normal lighting
#define LDR_MIN_BRIGHTNESS  5  // minimum brightness in auto mode (0–255)
```

Brightness is EMA-smoothed (¼ weight per 2 s sample) to prevent flickering.
Re-enabling auto mode snaps immediately to the current LDR reading.

### Battery ADC

The TC001 uses a large voltage divider — raw ADC range is 510–660, not a simple 1:2 ratio.

```cpp
#define BAT_RAW_EMPTY  510   // ADC reading at fully depleted (~3.0 V)
#define BAT_RAW_FULL   660   // ADC reading at fully charged (~4.2 V)
```

Watch `battery_raw` in `/api/status` at full and empty charge to recalibrate.

---

## Buttons

| Button | Action |
|---|---|
| **Left** (GPIO26) | Reserved — plays click sound |
| **Middle** (GPIO27) | Toggle auto-brightness on/off |
| **Right** (GPIO14) | Cycle display mode: clock → temp → humidity → battery → clock |

- Mode change via button times out back to clock after 10 s (auto-rotate manages its own timer).
- All buttons are debounced at 50 ms.
- Any button press exits the screensaver and resets the idle countdown.

---

## NVS Settings reference

All settings are stored in NVS namespace `ulanzi`. Defaults apply on first boot or after
flashing new firmware that adds a new key.

| Key | Type | Default | Setting |
|---|---|---|---|
| `auto_br` | bool | true | Auto-brightness enabled |
| `brightness` | uint8 | 50 | Manual brightness level |
| `boot_en` / `boot_vol` | bool / uint8 | true / 80 | Boot chime enable / volume |
| `poc_en` / `poc_vol` | bool / uint8 | true / 80 | POCSAG beep enable / volume |
| `clk_en` / `clk_vol` | bool / uint8 | true / 60 | Button click enable / volume |
| `rot_en` / `rot_sec` | bool / uint8 | false / 5 | Auto-rotate enable / interval |
| `icon_temp/hum/bat/poc` | string | built-in paths | Icon file paths |
| `ss_en` / `ss_timeout` / `ss_file` | bool / uint16 / string | false / 60 / "" | Screensaver |
| `col_clock` / `col_poc` | uint32 (RGB) | white / amber | Clock and POCSAG text colors |
| `t_thr_lo` / `t_thr_hi` | float | 16.0 / 20.0 | Temperature thresholds (°C) |
| `t_col_lo/mid/hi` | uint32 (RGB) | blue / green / orange | Temperature zone colors |
| `h_thr_lo` / `h_thr_hi` | float | 30.0 / 50.0 | Humidity thresholds (%) |
| `h_col_lo/mid/hi` | uint32 (RGB) | orange / green / blue | Humidity zone colors |
| `b_thr_lo` / `b_thr_hi` | uint8 | 30 / 60 | Battery thresholds (%) |
| `b_col_lo/mid/hi` | uint32 (RGB) | red / yellow / green | Battery zone colors |
| `boot_name` | string | `ULANZI` | Boot screen device name (max 8 chars, uppercase) |
| `mdns_name` | string | `ulanzi` | mDNS hostname → `<name>.local` |
| `ota_hostname` | string | `ulanzi-ota` | ArduinoOTA hostname (shown in Arduino IDE port list) |

---

## Implementation notes

- **`WiFi.setSleep(false)`** is required — WiFi power-save CPU pauses interrupt the RMT
  peripheral used by WS2812B, causing color glitches on every `FastLED.show()`.
- **`webTask` stack ≥ 8 KB** — `/api/status` allocates `char json[2500]` + `char logBuf[1100]`
  on the stack (~3.7 KB locals). Do not reduce below 6 KB.
- **`loadSettings()` before `drawBootScreen()`** — NVS (Preferences) is independent of
  LittleFS and can be read before the filesystem is mounted. Moving the call earlier ensures
  the saved boot name, brightness, and all other settings are applied before anything is
  displayed.
- **AnimatedGIF looping** — `playFrame` returns 1 while frames remain, 0 on the last frame
  of a non-looping GIF, and negative on a decode error. GIFs with the Netscape infinite-loop
  extension auto-rewind internally (never return 0). To make all GIFs loop regardless of
  their metadata, the code closes the file handle on `result == 0` so the next call to
  `_gifEnsureOpen` reopens from frame 0. `result < 0` returns `ICON_DRAW_FAILED` to trigger
  the bitmap fallback.
- **AnimatedGIF `_gif.begin()`** must be called before every `_gif.open()` — `_gif.close()`
  resets internal pixel byte order state.
- **TJpg_Decoder**: use the `getFsJpgSize(&w, &h, path, LittleFS)` overload — the
  `getFsJpgSize(File)` overload takes `fs::File` by value, whose destructor closes the
  shared file handle before `drawFsJpg` can use it.
- **I2C (DS1307 + SHT31)**: both devices share Wire and are accessed only from Core 1, so no
  mutex is needed.
- **POCSAG display state** is written only from Core 1 via `processPocsagPacket()` called
  in `loop()`. The ESP-NOW callback on Core 0 uses `xQueueSendFromISR` to avoid data races.
- **mDNS + OTA hostname** are both runtime-configurable and take effect immediately via
  `MDNS.end()` / `MDNS.begin()` and `ArduinoOTA.setHostname()`. No reboot required.
  The `OTA_HOSTNAME` define in `config.h` is no longer used; the runtime value from NVS
  (`ota_hostname`, default `ulanzi-ota`) is used instead.
