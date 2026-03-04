
# API Reference

All endpoints are served from the ESP32 webserver. Most return JSON unless otherwise noted.

## Bootlogos

### POST `/api/bootlogos/install?target=littlefs|sdcard`
- **Description:** Downloads and extracts the official bootlogos ZIP to `/bootlogos` on the selected storage.
- **Request:** No body. Query param `target` must be `littlefs` or `sdcard`.
- **Response:** `{ "status": "started" }` or error/status JSON.
- **Example:**
	```
	POST /api/bootlogos/install?target=littlefs
	```
- **Returns:** Starts install in background. Progress/status via `/api/bootlogos/status`.

### GET `/api/bootlogos/status`
- **Description:** Returns current install status.
- **Response:**
	```json
	{
		"active": true,
		"progress": 42,
		"files": 12,
		"status": "Extracting..."
	}
	```

## Snapshots

### GET `/api/snapshots/list?storage=sd|flash`
- **Description:** List all saved configuration snapshots.
- **Response:**
	```json
	{
		"storage": "sd",
		"mounted": true,
		"totalKB": 4096,
		"freeKB": 2048,
		"files": [
			{ "name": "backup1", "size": 1234 }
		]
	}
	```

### POST `/api/snapshots/save?storage=sd|flash&name=<name>`
- **Description:** Save current config as a named snapshot.
- **Response:** Plain text: `Saved '<name>' (N bytes)`

### POST `/api/snapshots/load?storage=sd|flash&name=<name>`
- **Description:** Load and apply a named snapshot.
- **Response:** Plain text: `Loaded '<name>'`

## Admin & System

### GET `/api/status`
- **Description:** Returns full system status (station, WiFi, Ethernet, MMDVM, etc.).
- **Response:** JSON with all status cards' data.

### GET `/api/logs`
- **Description:** Returns recent log messages.
- **Response:** JSON array of log entries.

### POST `/api/logs/clear`
- **Description:** Clears the log buffer.
- **Response:** Plain text: `OK`

### POST `/api/factory-reset`
- **Description:** Erases all settings and reboots.
- **Response:** Plain text: `Factory reset complete. Rebooting...`

### POST `/api/restart-mmdvm`
- **Description:** Restarts all digital mode tasks.
- **Response:** Plain text: `MMDVM restarted: N mode(s) started`

### POST `/api/restart-mqtt`
- **Description:** Restarts MQTT service.
- **Response:** Plain text: `MQTT service restarted`

### POST `/api/restart-services`
- **Description:** Restarts all mode tasks and MQTT.
- **Response:** Plain text: `Services restarted: ...`

### POST `/api/mode-toggle`
- **Description:** Enable/disable a radio mode.
- **Request:** Form params: `mode` (dmr, dstar, ysf, p25, nxdn, pocsag, dapnet), `enable` (true/false)
- **Response:** Plain text: `dmr mode enabled. Reboot required for changes to take effect.`

### POST `/api/reboot`
- **Description:** Reboots the device.
- **Response:** Plain text: `Rebooting...`

## DMR & Radio

### GET `/api/dmr-activity`
- **Description:** Returns current DMR call and last 15 calls.
- **Response:** JSON array.

### GET `/api/mode-status`
- **Description:** Returns enabled/connected status for all modes.
- **Response:** JSON array.

### GET `/api/service-status`
- **Description:** Returns enabled/connected status for all system services.
- **Response:** JSON array.

## WiFi

### GET `/api/wifiscan`
- **Description:** Scan for available WiFi networks.
- **Response:** JSON array of networks.

### GET `/api/get-wifi-slot?slot=N`
- **Description:** Get WiFi credentials for slot N (0-5).
- **Response:** JSON with label, ssid, password.

### POST `/api/save-wifi-slot`
- **Description:** Save WiFi credentials for a slot.
- **Request:** Form params: `slot`, `label`, `ssid`, `password`
- **Response:** Plain text: `WiFi slot N saved`

### POST `/api/reset-wifi-slot`
- **Description:** Reset WiFi slot to defaults.
- **Request:** Form param: `slot`
- **Response:** JSON with default values.

## Config Export/Import

### GET `/api/export-config`
- **Description:** Download all settings as a text file.
- **Response:** Text file (key:type=value per line).

### POST `/api/import-config`
- **Description:** Import settings from a text file.
- **Request:** Raw text body.
- **Response:** Plain text: `Configuration imported: N settings applied`

---

**Note:** All endpoints are subject to authentication if enabled. For full details, see the code and UI for each handler.
