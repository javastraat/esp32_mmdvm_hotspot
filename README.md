# ESP32 MMDVM Hotspot

<p align="center">
  <img src="images/esp32-mmdvm-logo.png" alt="ESP32 MMDVM Hotspot Logo" width="200"/>
</p>

A full-featured, FreeRTOS-based ESP32 firmware for a multi-mode digital voice modem hotspot. Designed for amateur radio operators, it supports DMR, POCSAG, and DAPNET out of the box, with a responsive web interface, MQTT integration, WireGuard VPN, SD card database support, and over-the-air firmware updates.

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
| **DMR** | ✅ Implemented | BrandMeister network, RadioID lookup, call history |
| **POCSAG** | ✅ Implemented | RF paging transmitter, queue management, whitelist/blacklist |
| **DAPNET** | ✅ Implemented | TCP paging network, message history, RIC filtering |
| D-Star | ⏳ Not yet in firmware | Framework in place |
| YSF (System Fusion) | ⏳ Not yet in firmware | Framework in place |
| P25 | ⏳ Not yet in firmware | Framework in place |
| NXDN | ⏳ Not yet in firmware | Framework in place |

---

## Features

### Radio & Protocols
- **DMR via BrandMeister** — login, keepalive, incoming/outgoing call handling
- **User database lookup** — in-memory cache (100 entries) + SD card SQLite + RadioID.net API fallback + QRZ.com integration
- **DMR call history** — last 15 calls with callsign, name, city, country
- **POCSAG paging transmitter** — numeric and alphanumeric, configurable frequency (~434 MHz)
- **DAPNET TCP client** — subscribes to DAPNET paging network, filters by RIC
- **CW ID** — periodic Morse code callsign transmission (configurable interval)

### Network Connectivity
- **WiFi** with 6 credential slots (auto-cycle on failure) + soft AP fallback (`MMDVM-Setup`)
- **Ethernet** (W5500 SPI module, configurable pins)
- **mDNS** hostname (default: `esp32-mmdvm.local`)
- **DNS fallback** server support
- **WireGuard VPN** client — tunnel to remote networks with private/public key management

### Time & Synchronization
- **NTP** time sync with timezone, DST offset, and configurable sync interval

### Web Interface
- **19-page responsive web UI** — accessible from any browser on the local network
- **Basic HTTP authentication** (default: `admin` / `pi-star`)
- **Configurable port** (default: 80)

### MQTT Integration
- **Publish:** status, logs, hardware info, DMR activity, POCSAG messages, per-task health
- **Subscribe:** command topic with optional token-based authorization
- **Remote commands:** `reboot`, `get_hardware`, `get_status`
- **Configurable:** broker, port, credentials, all topics individually

### Firmware Updates
- **ESP32 OTA** — stable, beta, or factory firmware downloaded from GitHub
- **MMDVM modem OTA** — UART bootloader flash (single/dual HAT support)
- **ArduinoOTA** — network upload from Arduino IDE (optional)
- **Dual OTA partition** — switch between `app0` and `app1`

### Storage
- **SD card** (SPI, configurable pins) — DMR user database (CSV + SQLite)
- **LittleFS** — configuration snapshots (internal flash)
- **NVS (Preferences)** — all runtime settings persisted across reboots
- **Factory reset** — clears NVS namespace, reboots to defaults

### Display & Indicators
- **OLED display** (I2C, 128×64, optional) — live call info and database lookups
- **LED** status indicator (configurable GPIO pin)
- **OLED frame buffer API** — export live display to web interface

### Monitoring & Logging
- **Circular log buffer** — 50 messages × 128 characters, thread-safe
- **System health** — chip temperature, heap usage, PSRAM, per-task free stack
- **NVS viewer & repair tools** — inspect and reset individual namespaces

### Configuration Management
- **Export/import** — download or upload all settings as a key=value text file
- **Snapshots** — save and restore named configuration snapshots (SD or LittleFS)

---

## Hardware Requirements

- **ESP32** (dual-core, tested on ESP32-S3 and ESP32)
- **MMDVM HAT** or compatible modem connected via UART
- *(Optional)* W5500 Ethernet module (SPI)
- *(Optional)* SSD1306 OLED display (I2C)
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
5. Configure your WiFi credentials under **Network → WiFi**
6. Set your callsign and DMR ID under **Modes → DMR**
7. Enable DMR and reboot — the hotspot will connect to BrandMeister

---

## Web Interface Pages

| Page | URL | Description |
|------|-----|-------------|
| Home | `/` | Overview and first-time setup wizard |
| Status | `/status` | System metrics, network, modem, MQTT, WireGuard |
| Mode Select | `/mode-select` | Enable/disable radio protocols |
| DMR | `/mode-dmr` | Callsign, ID, SSID, server, frequencies, color code |
| POCSAG / DAPNET | `/mode-pocsag` | Frequency, DAPNET server/auth, RIC, whitelist/blacklist |
| WiFi | `/wifi` | 6-slot credential management, AP settings, WiFi scan |
| Firmware | `/firmware` | OTA download/flash for ESP32 and modem |
| Admin | `/admin` | Logs, factory reset, service restart, reboot |
| MQTT | `/mqtt` | Broker, auth, topics, hardware interval, command token |
| WireGuard | `/wireguard` | Keys, endpoint, DNS, allowed IPs |
| SD Card | `/sdcard` | Status, database download/sync, file browser |
| Serial | `/serial` | MMDVM UART settings, baud rate, pins |
| System Info | `/info` | Chip model, memory, partitions |
| RF Settings | `/settings-mmdvm` | RX/TX frequency, color code, RF power, CW ID |

> Screenshots for every page coming soon.

---

## MQTT Commands

When MQTT is enabled, the device subscribes to the command topic (`mmdvm/command` by default) and processes commands in JSON format.

**Without token:**
```json
{"cmd": "reboot"}
```

**With token (if configured):**
```json
{"cmd": "reboot", "token": "your_token_here"}
```

| Command | Description |
|---------|-------------|
| `reboot` | Restart the ESP32 immediately |
| `get_hardware` | Publish detailed hardware info to the hardware topic |
| `get_status` | Publish current status and uptime to the status topic |

On connect, the device publishes an announce message to the command topic describing all available commands and whether a token is required.

---

## RTOS Architecture

The firmware runs on FreeRTOS with tasks distributed across both cores:

- **Core 0:** WiFi, Ethernet, Web Server, MQTT, NTP, SD Card, Logger, OLED, ArduinoOTA, WireGuard
- **Core 1:** Modem serial, DMR, D-Star, YSF, P25, NXDN, POCSAG, DAPNET

---

## Configuration

Settings are stored in NVS (Non-Volatile Storage) and persist across reboots. Compile-time defaults live in `include/config.h`. The priority order is:

1. NVS (set via web interface)
2. `config.h` defaults (compile-time fallback)

Configuration can be exported as a key=value file and re-imported, or saved as named snapshots on SD card or LittleFS.

---

## License

Amateur radio use. See LICENSE file for details.
