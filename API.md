# ESP32 MMDVM Hotspot — API Reference

All endpoints are served by the built-in HTTP server (default port 80). Write endpoints require HTTP Basic authentication (`admin` / `pi-star` by default). All responses are `text/plain` unless noted otherwise.

---

## Web Pages

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Home / setup wizard |
| GET | `/system-status` | System status dashboard |
| GET | `/system-settings` | Settings menu aggregator |
| GET | `/system-wifi` | WiFi credential management |
| GET | `/system-mqtt` | MQTT configuration |
| GET | `/system-wireguard` | WireGuard VPN configuration |
| GET | `/system-sdcard` | SD card status and database tools |
| GET | `/system-firmware` | OTA firmware update |
| GET | `/system-admin` | Admin: logs, factory reset, reboot |
| GET | `/system-serialmonitor` | MMDVM serial / UART settings |
| GET | `/system-info` | Chip and memory information |
| GET | `/mode-select` | Mode enable / disable |
| GET | `/mode-dmr` | DMR settings |
| GET | `/mode-pocsag` | POCSAG / DAPNET settings |
| GET | `/mode-dstar` | D-Star settings (not yet implemented) |
| GET | `/mode-ysf` | YSF settings (not yet implemented) |
| GET | `/mode-p25` | P25 settings (not yet implemented) |
| GET | `/mode-nxdn` | NXDN settings (not yet implemented) |
| GET | `/settings-mmdvm` | RF settings and CW ID |

---

## Static Assets

| Method | Path | Description |
|--------|------|-------------|
| GET | `/favicon.ico` | SVG favicon |
| GET | `/apple-touch-icon.png` | iOS home screen icon (180×180) |
| GET | `/pwa-icon-192.png` | PWA icon (192×192) |
| GET | `/pwa-icon-512.png` | PWA icon (512×512) |
| GET | `/manifest.json` | PWA web app manifest |

---

## System Status & Monitoring

| Method | Path | Response | Description |
|--------|------|----------|-------------|
| GET | `/api/status` | JSON | Full system status (network, heap, modem, MQTT, WireGuard, tasks) |
| GET | `/api/logs` | JSON | Circular log buffer (last 50 messages) |
| POST | `/api/logs/clear` | text | Clear the log buffer |
| GET | `/api/dmr-activity` | JSON | Current DMR call and last 15 call history entries |
| GET | `/api/service-status` | JSON | Enabled flags and runtime state for all services |
| GET | `/api/mode-status` | JSON | Enabled flags and connection state for all radio modes |

---

## System Admin

| Method | Path | Body | Description |
|--------|------|------|-------------|
| POST | `/api/reboot` | — | Reboot the ESP32 |
| POST | `/api/factory-reset` | — | Erase the `mmdvm` NVS namespace and reboot to defaults |
| POST | `/api/restart-mmdvm` | — | Stop and restart all mode tasks (DMR, POCSAG, etc.) |
| POST | `/api/restart-mqtt` | — | Restart the MQTT task |
| POST | `/api/restart-services` | — | Restart all services (modes + MQTT) |
| POST | `/api/mode-toggle` | `mode=<name>&enable=<1\|0>` | Enable or disable a radio mode at runtime |

---

## WiFi

| Method | Path | Params | Description |
|--------|------|--------|-------------|
| GET | `/api/wifiscan` | — | Scan and return available WiFi networks (JSON) |
| GET | `/api/get-wifi-slot` | `slot=<0-5>` | Get label, SSID, and password for a credential slot (JSON) |
| POST | `/api/save-wifi-slot` | `slot`, `label`, `ssid`, `password` | Save a WiFi credential slot to NVS |
| POST | `/api/reset-wifi-slot` | `slot=<0-5>` | Reset a slot to `config.h` defaults |

### WiFi AP & Network

| Method | Path | Params | Description |
|--------|------|--------|-------------|
| POST | `/api/save-wifi-ap-settings` | `ap_ssid`, `ap_password`, `ap_channel`, `max_retries` | Save soft AP settings |
| POST | `/api/reset-wifi-ap-settings` | — | Reset AP settings to defaults |
| POST | `/api/save-network-settings` | `mdns_enabled`, `mdns_hostname`, `web_port` | Save mDNS hostname and web server port |
| POST | `/api/reset-network-settings` | — | Reset mDNS / web settings to defaults |
| POST | `/api/save-eth-dns-settings` | `eth_enabled`, `dns_fallback_enabled`, `dns_fallback_ip` | Save Ethernet and DNS fallback settings |
| POST | `/api/reset-eth-dns-settings` | — | Reset Ethernet / DNS settings to defaults |

---

## NTP / Time

| Method | Path | Params | Description |
|--------|------|--------|-------------|
| POST | `/api/save-time-settings` | `ntp_enabled`, `ntp_server`, `gmt_offset`, `dst_offset`, `sync_interval` | Save NTP configuration |
| POST | `/api/reset-time-settings` | — | Reset NTP settings to `config.h` defaults |

---

## DMR Settings

| Method | Path | Params | Description |
|--------|------|--------|-------------|
| POST | `/api/save-callsign` | `callsign`, `dmr_id` | Save operator callsign and DMR ID |
| POST | `/api/save-station` | `dmr_ssid`, `dmr_colorcode`, `dmr_slot`, ... | Save station / slot settings |
| POST | `/api/reset-station` | — | Reset station settings to defaults |
| POST | `/api/save-dmr-network` | `dmr_server`, `dmr_password`, `rx_freq`, `tx_freq`, `rf_power` | Save DMR network and frequency settings |
| POST | `/api/reset-dmr-network` | — | Reset DMR network settings to defaults |
| POST | `/api/save-hotspot` | `hotspot_lat`, `hotspot_lon`, `hotspot_desc` | Save hotspot location and description |
| POST | `/api/reset-hotspot` | — | Reset hotspot info to defaults |

---

## RF Settings & CW ID

| Method | Path | Params | Description |
|--------|------|--------|-------------|
| POST | `/api/save-rf-settings` | `rx_freq`, `tx_freq`, `rf_power`, `color_code` | Save RF frequency and power settings |
| POST | `/api/reset-rf-settings` | — | Reset RF settings to defaults |
| POST | `/api/save-cwid-settings` | `cwid_enabled`, `cwid_callsign`, `cwid_interval` | Save CW ID settings |
| POST | `/api/reset-cwid-settings` | — | Reset CW ID settings to defaults |
| POST | `/api/test-cwid` | — | Trigger an immediate CW ID transmission |

---

## POCSAG & DAPNET

| Method | Path | Params / Response | Description |
|--------|------|-------------------|-------------|
| POST | `/api/send-pocsag` | `ric`, `message`, `type` | Send a POCSAG message (queued for RF transmission) |
| POST | `/api/save-pocsag-settings` | `pocsag_freq`, `dapnet_server`, `dapnet_port`, `dapnet_cs`, `dapnet_key`, `dapnet_ric`, `pocsag_wlist`, `pocsag_blist` | Save POCSAG and DAPNET settings |
| POST | `/api/reset-pocsag-settings` | — | Reset POCSAG / DAPNET settings to defaults |
| GET | `/api/pocsag-queue` | JSON | Current POCSAG transmit queue (`count`, `capacity`, `items`) |
| GET | `/api/dapnet-history` | JSON | Recent DAPNET received messages |
| GET | `/api/pocsag-tx-history` | JSON | Recent POCSAG transmit history |

---

## MQTT

| Method | Path | Params | Description |
|--------|------|--------|-------------|
| POST | `/api/save-mqtt-service` | `mqtt_enabled` | Enable or disable MQTT |
| POST | `/api/reset-mqtt-service` | — | Reset MQTT enabled flag to default |
| POST | `/api/save-mqtt-broker` | `mqtt_broker`, `mqtt_port` | Save broker hostname and port |
| POST | `/api/reset-mqtt-broker` | — | Reset broker settings to defaults |
| POST | `/api/save-mqtt-auth` | `mqtt_user`, `mqtt_password` | Save MQTT broker credentials |
| POST | `/api/reset-mqtt-auth` | — | Reset MQTT auth to defaults |
| POST | `/api/save-mqtt-topics` | `mqtt_status_topic`, `mqtt_log_topic`, `mqtt_hw_topic`, `mqtt_subscribe_topic` | Save MQTT topic names |
| POST | `/api/reset-mqtt-topics` | — | Reset topic names to defaults |
| POST | `/api/save-mqtt-advanced` | `mqtt_hw_info_log`, `mqtt_send_hw_info` | Save advanced MQTT settings |
| POST | `/api/reset-mqtt-advanced` | — | Reset advanced settings to defaults |
| GET | `/api/get-mqtt-token` | — | Get the current command token (plain text) |
| POST | `/api/save-mqtt-token` | `token=<value>` | Save command token to NVS (empty value disables MQTT commands entirely) |

### MQTT Command Channel

When MQTT is enabled the device subscribes to the configured command topic. **A token is always required** — commands without a matching token are rejected.

```json
{"cmd": "reboot",       "token": "your_token"}
{"cmd": "get_hardware", "token": "your_token"}
{"cmd": "get_status",   "token": "your_token"}
```

| Command | Description |
|---------|-------------|
| `reboot` | Restart the ESP32 |
| `get_hardware` | Publish hardware info JSON to the hardware topic |
| `get_status` | Publish `{"status":"online","uptime_seconds":N}` to the status topic |

On connect the device publishes an announce message listing all commands and confirming `token_required: true`.

---

## WireGuard

| Method | Path | Params | Description |
|--------|------|--------|-------------|
| POST | `/api/save-wg-service` | `wg_enabled` | Enable or disable WireGuard |
| POST | `/api/reset-wg-service` | — | Reset WireGuard enabled flag to default |
| POST | `/api/save-wg-config` | `wg_private_key`, `wg_public_key`, `wg_endpoint`, `wg_port`, `wg_allowed_ips`, `wg_dns` | Save WireGuard tunnel configuration |
| POST | `/api/reset-wg-config` | — | Reset WireGuard config to defaults |

---

## Hardware Settings

| Method | Path | Params | Description |
|--------|------|--------|-------------|
| GET | `/api/oled-framebuffer` | — | Raw OLED frame buffer (1024 bytes, 128×64 monochrome) |
| POST | `/api/save-led-button-settings` | `led_pin`, `button_pin` | Save LED and button GPIO pins |
| POST | `/api/reset-led-button-settings` | — | Reset LED / button pins to defaults |
| POST | `/api/save-oled-settings` | `oled_enabled`, `oled_sda`, `oled_scl`, `oled_addr` | Save OLED display settings |
| POST | `/api/reset-oled-settings` | — | Reset OLED settings to defaults |
| POST | `/api/save-eth-settings` | `eth_miso`, `eth_mosi`, `eth_sclk`, `eth_cs`, `eth_int` | Save Ethernet SPI pin settings |
| POST | `/api/reset-eth-settings` | — | Reset Ethernet pin settings to defaults |
| POST | `/api/save-sdcard-settings` | `sd_miso`, `sd_mosi`, `sd_sclk`, `sd_cs` | Save SD card SPI pin settings |
| POST | `/api/reset-sdcard-settings` | — | Reset SD card pin settings to defaults |
| POST | `/api/save-ota-settings` | `arduino_ota_enabled`, `arduino_ota_password` | Save ArduinoOTA settings |
| POST | `/api/reset-ota-settings` | — | Reset ArduinoOTA settings to defaults |
| POST | `/api/save-security-settings` | `web_user`, `web_password` | Save web authentication credentials |
| POST | `/api/reset-security-settings` | — | Reset web auth to defaults (`admin` / `pi-star`) |

---

## SD Card & Database

| Method | Path | Params | Description |
|--------|------|--------|-------------|
| GET | `/api/sdcard/status` | — | SD card mounted status (JSON) |
| GET | `/api/sdcard/info` | — | SD card type, size, and filesystem info (JSON) |
| GET | `/api/sdcard/files` | — | Recursive file listing on SD card (JSON) |
| GET | `/api/sdcard/owner` | — | Read `owner.txt` content |
| POST | `/api/sdcard/writeowner` | `owner=<content>` | Write `owner.txt` |
| GET | `/api/sdcard/download/csv` | — | Trigger download of DMR CSV database from RadioID.net |
| GET | `/api/sdcard/download/sqlite` | — | Trigger download of DMR SQLite database from RadioID.net |
| GET | `/api/sdcard/status/csv` | — | CSV download progress (JSON) |
| GET | `/api/sdcard/status/sqlite` | — | SQLite download progress (JSON) |
| GET | `/api/sdcard/delete/csv` | — | Delete the CSV database file |
| GET | `/api/sdcard/delete/sqlite` | — | Delete the SQLite database file |
| GET | `/api/sdcard/delete/custom` | `path=<filepath>` | Delete a file at a custom path on the SD card |
| GET | `/api/dmr/user/` | `id=<dmr_id>` | Look up a DMR user by ID (JSON) |
| GET | `/api/sqlite/search` | `q=<query>` | Full-text search in the SQLite DMR database (JSON) |

---

## OTA Firmware Updates

### ESP32 Firmware

| Method | Path | Params | Description |
|--------|------|--------|-------------|
| POST | `/download-update` | `channel=<stable\|beta\|factory>` | Download firmware binary from GitHub to SPIFFS |
| POST | `/flash-firmware` | — | Flash the previously downloaded binary to the inactive OTA partition |
| POST | `/cancel-flash` | — | Cancel an in-progress download or flash |
| POST | `/upload-firmware` | multipart binary | Upload and flash a firmware binary directly via HTTP |
| POST | `/switch-partition` | — | Switch the active OTA boot partition (`app0` ↔ `app1`) |

### MMDVM Modem Firmware

| Method | Path | Params | Description |
|--------|------|--------|-------------|
| GET | `/get-modem-version` | — | Query the modem firmware version via UART |
| GET | `/flash-modem-status` | — | Get modem flash progress / status (JSON) |
| POST | `/flash-modem-url` | `url=<firmware_url>` | Download modem firmware from URL and flash via UART bootloader |
| POST | `/flash-modem-upload` | multipart binary | Upload modem firmware binary and flash via UART bootloader |
| POST | `/api/save-modem-settings` | `modem_baud`, `modem_tx_pin`, `modem_rx_pin`, `modem_boot_pin`, `modem_reset_pin` | Save MMDVM modem UART settings |
| POST | `/api/reset-modem-settings` | — | Reset modem UART settings to defaults |

---

## Configuration Export / Import

| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/export-config` | Download all NVS settings as a `key=value` text file |
| POST | `/api/import-config` | Upload a `key=value` text file to bulk-restore settings |

---

## Configuration Snapshots

| Method | Path | Params | Description |
|--------|------|--------|-------------|
| GET | `/api/snapshots/list` | `storage=<sd\|flash>` | List saved snapshots (JSON) |
| POST | `/api/snapshots/save` | `name=<snapshot_name>` | Save current config as a named snapshot |
| POST | `/api/snapshots/load` | `name=<snapshot_name>` | Load and apply a saved snapshot |
| POST | `/api/snapshots/delete` | `name=<snapshot_name>` | Delete a saved snapshot |
| GET | `/api/snapshots/download` | `name=<snapshot_name>` | Download a snapshot file |

---

## NVS Management

| Method | Path | Params | Description |
|--------|------|--------|-------------|
| GET | `/api/show-prefs` | — | Display all NVS keys and values in the `mmdvm` namespace (HTML table) |
| GET | `/api/show-prefs-raw` | `namespace=<ns>` | Enumerate NVS keys using the ESP-IDF iterator (JSON) |
| POST | `/api/repair-prefs` | — | Scan NVS and add any missing keys with `config.h` defaults |
| GET | `/api/list-nvs-namespaces` | — | List all NVS namespaces found on the device (JSON) |
| POST | `/api/prefs-reset` | — | Erase all keys in the `mmdvm` NVS namespace (does not reboot) |
