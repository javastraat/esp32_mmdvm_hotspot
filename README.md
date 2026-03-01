# ESP32 MMDVM Hotspot

<p align="center">
  <img src="images/esp32-mmdvm-logo.png" alt="ESP32 MMDVM Hotspot Logo" width="200"/>
</p>

A FreeRTOS-based ESP32 firmware for a multi-mode digital voice modem hotspot. Designed for amateur radio operators, it currently supports DMR (network→RF transmit), POCSAG, and DAPNET, with a responsive web interface, MQTT integration, WireGuard VPN, SD card database support, and over-the-air firmware updates.

---

## Current Firmware Version

| Channel | Version |
|---------|---------|
| Stable  | `20260301_ESP32` |
| Beta    | `20260301_ESP32_BETA` |

---

## Implemented Modes

| Mode | Status | Notes |
|------|--------|-------|
| **DMR** | ⚠️ Partial | Network→RF TX working. RF→Network RX is work in progress. |
| **POCSAG** | ✅ Implemented | RF paging transmitter, queue management, whitelist/blacklist |
| **DAPNET** | ✅ Implemented | TCP paging network, message history, RIC filtering |
| D-Star | ❌ Not in firmware | Framework stub only |
| YSF (System Fusion) | ❌ Not in firmware | Framework stub only |
| P25 | ❌ Not in firmware | Framework stub only |
| NXDN | ❌ Not in firmware | Framework stub only |

---

## Features

### Radio & Protocols

- **DMR via BrandMeister (network→RF)** — full login state machine (RPTL/RPTK/RPTC), keepalive (RPTPING/MSTPONG), incoming voice frame buffering and paced transmission to the MMDVM modem
- **DMR RF→Network receive** — frame parsing from modem serial is in progress; not yet relayed back to BrandMeister
- **DMR Talker Alias extraction** — embedded LC decoded in real time (QR(16,7,6) + Hamming(16,11,4) + CRC5) to display the caller's alias on the OLED before the database lookup completes
- **DMR user lookup** — in-memory LRU cache (100 entries) → SD card SQLite → RadioID.net API fallback, all performed asynchronously so keepalives are not delayed
- **DMR call history** — last 15 calls with callsign, name, city, and country
- **POCSAG paging transmitter** — numeric and alphanumeric messages, BCH(32,21) encoding, configurable frequency and functional codes, up to 32-message queue with TX history
- **DAPNET TCP client** — connects to DAPNET network, HMAC-MD5 authentication, RIC filtering, 15-message receive history
- **CW ID** — periodic Morse code callsign transmission at a configurable interval, with a manual test trigger via the web UI

### Network Connectivity

- **WiFi** with 6 labeled credential slots, automatic slot cycling on failure, soft AP fallback (`MMDVM-Setup`)
- **Ethernet** — W5500 SPI module with configurable pins
- **mDNS** hostname resolution (default: `esp32-mmdvm.local`)
- **DNS fallback** server support
- **WireGuard VPN** client — ChaCha20-Poly1305 encrypted tunnel, configurable peer endpoint, allowed IPs, and DNS

### Time & Synchronisation

- **NTP** time sync with configurable server, GMT offset, DST offset, and sync interval

### Web Interface

- **20-page responsive web UI** — accessible from any browser on the local network
- **HTTP Basic Auth** (default: `admin` / `pi-star`, fully configurable)
- **Configurable port** (default 80)
- **PWA support** — installable as a progressive web app (manifest + icons)

### MQTT Integration

- **Publish:** system status, log stream, hardware info, DMR activity, POCSAG events, per-task heartbeats
- **Subscribe:** command topic — token authentication is enforced (a token must be configured and included with every command)
- **Remote commands:** `reboot`, `get_hardware`, `get_status`
- **Configurable:** broker address, port, credentials, all topic names individually, hardware info publish interval
- **Command announce** — publishes available commands and token requirement on each connect

### Firmware Updates

- **ESP32 OTA** — stable, beta, or factory image downloaded from GitHub and flashed to the inactive OTA partition
- **MMDVM modem OTA** — UART bootloader flash from URL or direct HTTP upload (single and dual-HAT support)
- **ArduinoOTA** — optional network upload from the Arduino IDE
- **Dual OTA partition** — manual partition switch (`app0` ↔ `app1`) via the web UI

### Storage

- **SD card** (SPI, configurable pins) — DMR user database in CSV and SQLite formats
- **LittleFS** — named configuration snapshots stored in internal flash
- **NVS (Preferences)** — all runtime settings persisted across reboots under the `mmdvm` namespace
- **Factory reset** — erases the NVS namespace and reboots to `config.h` defaults

### Display & Indicators

- **OLED display** (I2C, 128×64, SSD1306/SH1106, optional) — live call info, Talker Alias, and async database lookup results
- **LED** status indicator on a configurable GPIO pin
- **OLED frame buffer API** — the live 128×64 monochrome display can be fetched over HTTP

### Monitoring & Logging

- **Circular log buffer** — 50 messages × 128 characters, thread-safe, accessible via API and the serial monitor page
- **System health** — ESP32 chip temperature, heap usage, PSRAM, and per-task free stack space
- **NVS viewer & repair tools** — inspect all NVS namespaces, add missing keys from defaults, or erase the `mmdvm` namespace

### Configuration Management

- **Export / import** — download or upload all settings as a `key=value` text file
- **Named snapshots** — save and restore complete configuration snapshots to SD card or LittleFS

---

## Hardware Requirements

- **ESP32** (dual-core; tested on ESP32 and ESP32-S3)
- **MMDVM HAT** or compatible modem connected via UART
- *(Optional)* W5500 Ethernet module (SPI)
- *(Optional)* SSD1306/SH1106 OLED display (I2C, 128×64)
- *(Optional)* MicroSD card module (SPI)

---

## Default Pin Configuration

| Function | Pin | Notes |
|----------|-----|-------|
| MMDVM TX | 43 | UART to modem |
| MMDVM RX | 44 | UART from modem |
| MMDVM Boot | 4 | Bootloader trigger |
| MMDVM Reset | 13 | Hard reset |
| OLED SDA | 17 | I2C data |
| OLED SCL | 18 | I2C clock |
| LED | 38 | Status indicator |
| Button | 0 | OLED toggle |
| SD MISO | 9 | SPI |
| SD MOSI | 11 | SPI |
| SD SCLK | 10 | SPI |
| SD CS | 12 | SPI |
| ETH MISO | 47 | W5500 SPI |
| ETH MOSI | 21 | W5500 SPI |
| ETH SCLK | 48 | W5500 SPI |
| ETH CS | 45 | W5500 SPI |
| ETH INT | 14 | W5500 interrupt |

All pins are configurable via the web interface.

---

## Default Credentials

| Setting | Default |
|---------|---------|
| Web username | `admin` |
| Web password | `pi-star` |
| WiFi AP SSID | `MMDVM-Setup` |
| WiFi AP password | `mmdvm1234` |
| ArduinoOTA password | `mmdvm` |
| DMR callsign | `N0CALL` |
| DMR ID | `1234567` |

---

## Getting Started

1. Flash the firmware to your ESP32
2. Connect to the `MMDVM-Setup` WiFi access point (password: `mmdvm1234`)
3. Open a browser and navigate to `http://192.168.4.1`
4. Log in with `admin` / `pi-star`
5. Go to **Network → WiFi** and configure your home WiFi credentials
6. Go to **Modes → DMR** and set your callsign and DMR ID
7. Go to **Mode Select** and enable DMR
8. Reboot — the hotspot will connect to BrandMeister

---

## Web Interface Pages

| Page | URL | Description |
|------|-----|-------------|
| Home | `/` | Overview and first-time setup wizard |
| Status | `/system-status` | System metrics, network, modem, MQTT, WireGuard |
| Settings | `/system-settings` | Settings menu aggregator |
| Mode Select | `/mode-select` | Enable / disable radio protocols |
| DMR | `/mode-dmr` | Callsign, DMR ID, SSID, server, frequencies, color code |
| POCSAG / DAPNET | `/mode-pocsag` | Frequency, DAPNET server/auth, RIC, whitelist/blacklist |
| D-Star | `/mode-dstar` | Placeholder — not yet implemented |
| YSF | `/mode-ysf` | Placeholder — not yet implemented |
| P25 | `/mode-p25` | Placeholder — not yet implemented |
| NXDN | `/mode-nxdn` | Placeholder — not yet implemented |
| WiFi | `/system-wifi` | 6-slot credential management, AP settings, WiFi scan |
| Firmware | `/system-firmware` | OTA download/flash for ESP32 and modem |
| Admin | `/system-admin` | Logs, factory reset, service restart, reboot |
| MQTT | `/system-mqtt` | Broker, auth, topics, hardware interval, command token |
| WireGuard | `/system-wireguard` | Keys, endpoint, DNS, allowed IPs |
| SD Card | `/system-sdcard` | Status, database download/sync, file browser |
| Serial Monitor | `/system-serialmonitor` | MMDVM UART settings, baud rate, pins |
| System Info | `/system-info` | Chip model, memory, partitions |
| RF Settings | `/settings-mmdvm` | RX/TX frequency, color code, RF power, CW ID |

---

## MQTT Commands

When MQTT is enabled the device subscribes to the configured command topic. **A token is always required** — commands without a valid token are rejected. Configure a token under **System → MQTT → Command Token**.

```json
{"cmd": "reboot", "token": "your_token"}
{"cmd": "get_hardware", "token": "your_token"}
{"cmd": "get_status", "token": "your_token"}
```

| Command | Description |
|---------|-------------|
| `reboot` | Restart the ESP32 immediately |
| `get_hardware` | Publish hardware info JSON to the hardware topic |
| `get_status` | Publish `{"status":"online","uptime_seconds":N}` to the status topic |

On connect the device publishes an announce message listing all available commands and confirming that a token is required.

---

## RTOS Architecture

The firmware runs on FreeRTOS with tasks pinned to specific cores:

- **Core 0:** WiFi, Ethernet, Web Server, MQTT, NTP, SD Card, Logger, OLED, ArduinoOTA, WireGuard
- **Core 1:** Modem serial, DMR, POCSAG, DAPNET, D-Star (stub), YSF (stub), P25 (stub), NXDN (stub)

A separate low-priority **DMR Database** task runs on Core 0 and performs blocking SQLite / HTTP lookups asynchronously so DMR keepalives are never delayed.

---

## Configuration

Settings are stored in NVS (Non-Volatile Storage) and persist across reboots. Compile-time defaults live in `include/config.h`. Priority order:

1. NVS (set via web interface or config import)
2. `config.h` defaults (compile-time fallback)

Configuration can be exported as a `key=value` text file and re-imported, or saved as named snapshots on SD card or LittleFS.

---

## License

Amateur radio use. See LICENSE file for details.
