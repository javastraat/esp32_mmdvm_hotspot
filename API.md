# ESP32 MMDVM Hotspot — API Reference

All endpoints are served by the onboard HTTP web server (default port 80).
Authentication is required unless disabled. Pass credentials via HTTP Basic Auth.

Default credentials: `admin` / `pi-star`

---

## Table of Contents

- [System & Status](#system--status)
- [WiFi](#wifi)
- [Network & NTP](#network--ntp)
- [DMR & Radio](#dmr--radio)
- [Modes](#modes)
- [POCSAG & DAPNET](#pocsag--dapnet)
- [MQTT](#mqtt)
- [WireGuard](#wireguard)
- [Hardware](#hardware)
- [SD Card](#sd-card)
- [OTA Firmware](#ota-firmware)
- [Configuration](#configuration)
- [Security & Admin](#security--admin)
- [NVS Management](#nvs-management)
- [MQTT Commands](#mqtt-commands)

---

## System & Status

### `GET /api/status`
Returns a JSON object with current system status.

**Response fields:** uptime, free heap, chip temperature, WiFi SSID/RSSI/IP, Ethernet status, modem firmware version, MQTT connected, WireGuard status.

---

### `GET /api/logs`
Returns the last 50 log messages as a JSON array.

---

### `POST /api/logs/clear`
Clears the in-memory log buffer.

---

### `GET /api/dmr-activity`
Returns current DMR call info and the last 15 call history entries as JSON.

**Response fields per entry:** callsign, name, city, country, dmr_id, slot, talkgroup, start_time.

---

### `GET /api/mode-status`
Returns enabled flags and connection state for each protocol mode.

**Response fields:** dmr_enabled, dmr_connected, dstar_enabled, ysf_enabled, p25_enabled, nxdn_enabled, pocsag_enabled, dapnet_enabled, dapnet_connected.

---

### `GET /api/service-status`
Returns status of optional services.

**Response fields:** eth_connected, mdns_active, ntp_synced, wg_connected.

---

## WiFi

### `GET /api/wifiscan`
Triggers a WiFi scan and returns nearby networks.

**Response fields per entry:** ssid, rssi, encryption.

---

### `GET /api/get-wifi-slot`
Returns credentials for a specific WiFi slot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `slot` | int (0–5) | Slot index |

**Response fields:** label, ssid, password.

---

### `POST /api/save-wifi-slot`
Saves WiFi credentials to a slot. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `slot` | int (0–5) | Slot index |
| `label` | string | Display name |
| `ssid` | string | Network SSID |
| `password` | string | Network password |

---

### `POST /api/reset-wifi-slot`
Resets a slot to its `config.h` default.

| Parameter | Type | Description |
|-----------|------|-------------|
| `slot` | int (0–5) | Slot index |

---

## Network & NTP

### `POST /api/save-network-settings`
Saves mDNS and web server settings. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `mdns` | `0`/`1` | Enable mDNS |
| `hostname` | string | mDNS hostname (no `.local`) |
| `dnsfallback` | `0`/`1` | Enable DNS fallback |
| `dnsfallbackip` | string | Fallback DNS IP address |

---

### `POST /api/reset-network-settings`
Resets network settings to `config.h` defaults. Triggers reboot.

---

### `POST /api/save-time-settings`
Saves NTP configuration. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ntpenabled` | `0`/`1` | Enable NTP |
| `server` | string | NTP server hostname |
| `gmt` | int | GMT offset in seconds |
| `dst` | int | DST offset in seconds |
| `sync` | int | Sync interval in milliseconds |

---

### `POST /api/reset-time-settings`
Resets NTP settings to defaults. Triggers reboot.

---

### `POST /api/save-eth-dns-settings`
Saves Ethernet and DNS fallback settings. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ethenabled` | `0`/`1` | Enable Ethernet |
| `ethdebug` | `0`/`1` | Enable Ethernet debug logging |
| `dnsfallback` | `0`/`1` | Enable DNS fallback |
| `dnsfallbackip` | string | Fallback DNS server IP |

---

### `POST /api/reset-eth-dns-settings`
Resets Ethernet and DNS settings to defaults. Triggers reboot.

---

### `POST /api/save-wifi-ap-settings`
Saves the soft AP fallback configuration. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ssid` | string | AP SSID |
| `password` | string | AP password (min 8 chars) |
| `channel` | int | WiFi channel (1–13) |
| `retries` | int | Connection attempts before AP fallback |

---

### `POST /api/reset-wifi-ap-settings`
Resets soft AP settings to defaults. Triggers reboot.

---

## DMR & Radio

### `POST /api/save-callsign`
Updates the user callsign only.

| Parameter | Type | Description |
|-----------|------|-------------|
| `callsign` | string | Amateur radio callsign |

---

### `POST /api/save-station`
Saves station identity settings. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `callsign` | string | Amateur radio callsign |
| `dmrid` | int | DMR radio ID |
| `ssid` | int | DMR SSID (0–99) |

---

### `POST /api/reset-station`
Resets station settings to defaults. Triggers reboot.

---

### `POST /api/save-dmr-network`
Saves DMR network and frequency settings. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `server` | string | BrandMeister server hostname/IP |
| `password` | string | BrandMeister password |
| `rxfreq` | int | RX frequency in Hz |
| `txfreq` | int | TX frequency in Hz |
| `colorcode` | int | DMR color code (0–15) |
| `rfpower` | int | RF power (0–255) |

---

### `POST /api/reset-dmr-network`
Resets DMR network settings to defaults. Triggers reboot.

---

### `POST /api/save-hotspot`
Saves hotspot location and description for BrandMeister registration.

| Parameter | Type | Description |
|-----------|------|-------------|
| `hs_callsign` | string | Hotspot callsign |
| `hs_suffix` | string | Suffix (e.g. `HS`) |
| `hs_latitude` | string | GPS latitude |
| `hs_longitude` | string | GPS longitude |
| `hs_height` | int | Antenna height (meters) |
| `hs_location` | string | Location description |
| `hs_desc` | string | Hotspot description |
| `hs_url` | string | Info URL |

---

### `POST /api/reset-hotspot`
Resets hotspot info to defaults. Triggers reboot.

---

### `POST /api/save-rf-settings`
Saves RF parameters. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `rxfreq` | int | RX frequency in Hz |
| `txfreq` | int | TX frequency in Hz |
| `colorcode` | int | DMR color code (0–15) |
| `rfpower` | int | RF power level (0–255) |

---

### `POST /api/reset-rf-settings`
Resets RF settings to defaults. Triggers reboot.

---

### `POST /api/save-cwid-settings`
Saves CW ID configuration.

| Parameter | Type | Description |
|-----------|------|-------------|
| `enabled` | `0`/`1` | Enable periodic CW ID |
| `interval` | int | Interval in minutes (1–60) |

---

### `POST /api/reset-cwid-settings`
Resets CW ID settings to defaults.

---

### `POST /api/test-cwid`
Transmits an immediate CW ID test using the current callsign.

---

## Modes

### `POST /api/mode-toggle`
Enables or disables a specific radio protocol. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `mode` | string | `dmr`, `dstar`, `ysf`, `p25`, `nxdn`, `pocsag`, or `dapnet` |
| `enabled` | `0`/`1` | Enable or disable |

---

### `POST /api/restart-mmdvm`
Stops and restarts all enabled protocol tasks without a full device reboot.

---

## POCSAG & DAPNET

### `POST /api/send-pocsag`
Queues a POCSAG paging message for transmission.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ric` | int | Pager RIC (recipient address) |
| `func` | int | Function bits (0–3) |
| `message` | string | Message text |

---

### `GET /api/pocsag-queue`
Returns the current POCSAG transmission queue as JSON.

---

### `GET /api/dapnet-history`
Returns the last 15 received DAPNET messages as JSON.

**Response fields per entry:** ric, message, timestamp.

---

### `GET /api/pocsag-tx-history`
Returns the POCSAG transmission history as JSON.

---

### `POST /api/save-pocsag-settings`
Saves POCSAG and DAPNET configuration. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `pocsag_freq` | int | POCSAG RF frequency in Hz |
| `dapnet_server` | string | DAPNET server hostname |
| `dapnet_port` | int | DAPNET server port |
| `dapnet_cs` | string | DAPNET node callsign |
| `dapnet_key` | string | DAPNET authentication key |
| `dapnet_ric` | string | RIC to receive (comma-separated) |
| `pocsag_wlist` | string | Whitelist RICs (comma-separated, empty = allow all) |
| `pocsag_blist` | string | Blacklist RICs (comma-separated) |

---

### `POST /api/reset-pocsag-settings`
Resets POCSAG and DAPNET settings to defaults. Triggers reboot.

---

## MQTT

### `POST /api/save-mqtt-service`
Enables or disables the MQTT client. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `enabled` | `0`/`1` | Enable MQTT |

---

### `POST /api/reset-mqtt-service`
Resets MQTT enabled flag to default. Triggers reboot.

---

### `POST /api/save-mqtt-broker`
Saves MQTT broker connection settings. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `broker` | string | Broker hostname or IP (min 3 chars) |
| `port` | int | Broker port (1–65535) |

---

### `POST /api/reset-mqtt-broker`
Resets broker settings to defaults. Triggers reboot.

---

### `POST /api/save-mqtt-auth`
Saves MQTT authentication credentials. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `user` | string | MQTT username (leave empty for no auth) |
| `pass` | string | MQTT password |

---

### `POST /api/reset-mqtt-auth`
Resets MQTT authentication to defaults. Triggers reboot.

---

### `POST /api/save-mqtt-topics`
Saves MQTT topic configuration. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `status` | string | Status publish topic |
| `logs` | string | Log publish topic |
| `hw` | string | Hardware info publish topic |
| `sub` | string | Command subscribe topic |

---

### `POST /api/reset-mqtt-topics`
Resets all topics to defaults. Triggers reboot.

---

### `POST /api/save-mqtt-advanced`
Saves advanced MQTT settings. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `interval` | int | Hardware info publish interval in seconds (5–3600) |
| `hwlog` | `0`/`1` | Log MQTT publishes to serial |

---

### `POST /api/reset-mqtt-advanced`
Resets advanced settings to defaults. Triggers reboot.

---

### `GET /api/get-mqtt-token`
Returns the current command token as plain text. Empty string if no token is set.

---

### `POST /api/save-mqtt-token`
Saves the MQTT command token to NVS. Takes effect immediately — no reboot required.

| Parameter | Type | Description |
|-----------|------|-------------|
| `token` | string | Token string. Leave empty to disable token check. |

**Response:** `Token saved` or `Token cleared (no token check)`

---

## WireGuard

### `POST /api/save-wg-settings`
Saves WireGuard VPN configuration. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `enabled` | `0`/`1` | Enable WireGuard |
| `localip` | string | Local tunnel IP (e.g. `10.0.0.2`) |
| `privkey` | string | Private key (base64) |
| `pubkey` | string | Server public key (base64) |
| `endpoint` | string | Server hostname or IP |
| `port` | int | Server UDP port (default: 51820) |
| `dns` | string | DNS server inside tunnel |
| `allowedips` | string | Allowed IPs (e.g. `0.0.0.0/0`) |

---

### `POST /api/reset-wg-settings`
Resets WireGuard settings to defaults. Triggers reboot.

---

## Hardware

### `GET /api/oled-framebuffer`
Returns the raw 1024-byte OLED frame buffer as binary data (128×64 pixels, 1 bit per pixel).

---

### `POST /api/save-oled-settings`
Saves OLED display configuration. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `oledenabled` | `0`/`1` | Enable OLED |
| `i2csda` | int | SDA GPIO pin |
| `i2cscl` | int | SCL GPIO pin |
| `oledaddr` | int | I2C address (e.g. `0x3C`) |
| `oledwidth` | int | Display width in pixels |
| `oledheight` | int | Display height in pixels |

---

### `POST /api/reset-oled-settings`
Resets OLED settings to defaults. Triggers reboot.

---

### `POST /api/save-led-button-settings`
Saves LED and button GPIO pin configuration. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ledpin` | int | LED GPIO pin |
| `buttonpin` | int | Button GPIO pin |

---

### `POST /api/reset-led-button-settings`
Resets LED/button pins to defaults. Triggers reboot.

---

### `POST /api/save-eth-settings`
Saves Ethernet SPI pin configuration. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ethcs` | int | W5500 CS pin |
| `ethmiso` | int | SPI MISO pin |
| `ethmosi` | int | SPI MOSI pin |
| `ethsclk` | int | SPI SCLK pin |
| `ethint` | int | Interrupt pin |
| `ethrst` | int | Reset pin (-1 to disable) |

---

### `POST /api/reset-eth-settings`
Resets Ethernet pin settings to defaults. Triggers reboot.

---

### `POST /api/save-sdcard-settings`
Saves SD card SPI pin configuration. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `sdmiso` | int | SPI MISO pin |
| `sdmosi` | int | SPI MOSI pin |
| `sdsclk` | int | SPI SCLK pin |
| `sdcs` | int | CS pin |

---

### `POST /api/reset-sdcard-settings`
Resets SD card pin settings to defaults. Triggers reboot.

---

## SD Card

### `GET /api/sdcard-status`
Returns SD card mount status, total size, and used space as JSON.

---

### `POST /api/sdcard-format`
Erases and reformats the SD card. **Destructive — all data is lost.**

---

### `GET /api/sdcard-ls`
Returns a directory listing as JSON.

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | string | Directory path to list |

---

### `POST /api/sdcard-download`
Downloads a DMR user database from the configured URL and saves to SD card.

| Parameter | Type | Description |
|-----------|------|-------------|
| `dbtype` | string | `csv` or `sqlite` |

---

## OTA Firmware

### `POST /download-update`
Downloads an ESP32 firmware binary to the inactive OTA partition.

| Parameter | Type | Description |
|-----------|------|-------------|
| `version` | string | `stable`, `beta`, `factory`, or `rtos` |

---

### `POST /api/confirm-update`
Flashes the previously downloaded firmware and reboots.

---

### `POST /flash-modem`
Downloads and flashes MMDVM modem firmware via UART bootloader.

| Parameter | Type | Description |
|-----------|------|-------------|
| `url` | string | Direct URL to modem firmware binary |

---

### `POST /api/switch-partition`
Switches the active OTA boot partition and reboots.

| Parameter | Type | Description |
|-----------|------|-------------|
| `partition` | string | `app0` or `app1` |

---

### `GET /api/check-update`
Checks the current firmware version against the remote version files.

**Response fields:** current_version, latest_stable, latest_beta, update_available, update_url.

---

## Configuration

### `GET /api/export-config`
Downloads all current settings as a plain-text `key=value` file.

---

### `POST /api/import-config`
Parses and applies settings from a `key=value` text body. Triggers reboot.

| Body | Type | Description |
|------|------|-------------|
| Raw text | text/plain | `key=value` pairs, one per line |

---

### `GET /api/snapshots/list`
Returns a list of saved configuration snapshots.

| Parameter | Type | Description |
|-----------|------|-------------|
| `storage` | string | `sd` or `flash` |

---

### `POST /api/snapshots/save`
Saves the current configuration as a named snapshot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | string | Snapshot name |
| `storage` | string | `sd` or `flash` |

---

### `POST /api/snapshots/load`
Restores settings from a named snapshot. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | string | Snapshot name |
| `storage` | string | `sd` or `flash` |

---

## Security & Admin

### `POST /api/save-security-settings`
Saves web interface credentials and server settings. Triggers reboot.

| Parameter | Type | Description |
|-----------|------|-------------|
| `username` | string | Web UI username |
| `password` | string | Web UI password |
| `webenabled` | `0`/`1` | Enable web server |
| `webport` | int | HTTP server port |

---

### `POST /api/reset-security-settings`
Resets web credentials and port to defaults. Triggers reboot.

---

### `POST /api/factory-reset`
Clears the entire NVS `mmdvm` namespace and reboots. **All settings are lost.**

---

### `POST /api/reboot`
Gracefully reboots the ESP32.

---

### `POST /api/restart-mqtt`
Restarts the MQTT client task without a full reboot.

---

### `POST /api/restart-services`
Restarts all system service tasks (WiFi, Ethernet, NTP, MQTT, Web Server).

---

## NVS Management

### `GET /api/list-nvs-namespaces`
Returns all keys and values in the NVS `mmdvm` namespace as JSON. Useful for debugging.

---

### `POST /api/reset-prefs`
Clears all keys in a specific NVS namespace.

| Parameter | Type | Description |
|-----------|------|-------------|
| `namespace` | string | NVS namespace name to clear |

---

## MQTT Commands

The device subscribes to the configured command topic (default: `mmdvm/command`) and accepts JSON commands. If a token is configured, it must be included in every command.

**Format without token:**
```json
{"cmd": "<command>"}
```

**Format with token:**
```json
{"cmd": "<command>", "token": "<your_token>"}
```

### Available Commands

| Command | Description | Response Topic |
|---------|-------------|----------------|
| `reboot` | Restart the ESP32 immediately | `mmdvm/status` → `{"status":"rebooting"}` |
| `get_hardware` | Publish hardware info JSON | `mmdvm/hardware` |
| `get_status` | Publish uptime and online status | `mmdvm/status` |

### Announce Message

On every MQTT connect, the device publishes an announce message to the command topic:

```json
{
  "info": "Commands available",
  "commands": ["reboot", "get_hardware", "get_status"],
  "token_required": true,
  "usage": {"cmd": "<command>", "token": "<your_token>"},
  "example": {"cmd": "reboot", "token": "<your_token>"},
  "client": "esp32-mmdvm-XXXXXXXX"
}
```

Token check can be disabled by clearing the token via `POST /api/save-mqtt-token` with an empty value.

### Default MQTT Topics

| Topic | Direction | Content |
|-------|-----------|---------|
| `mmdvm/status` | Publish | Connection status, reboot notifications, get_status responses |
| `mmdvm/logs` | Publish | Log messages (when log publishing enabled) |
| `mmdvm/hardware` | Publish | Hardware info JSON (periodic + on `get_hardware` command) |
| `mmdvm/command` | Subscribe | Incoming commands |
| `mmdvm/task/dmr` | Publish | DMR task health |
| `mmdvm/task/pocsag` | Publish | POCSAG task health |
| `mmdvm/task/dapnet` | Publish | DAPNET task health |
| `mmdvm/task/modem` | Publish | Modem task health |
| `mmdvm/task/wifi` | Publish | WiFi task health |
| `mmdvm/task/mqtt_client` | Publish | MQTT task health |
| `mmdvm/task/*` | Publish | All other task health topics |
