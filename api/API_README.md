# ESP32 MMDVM Hotspot API Documentation

## Overview

This document describes all available HTTP API endpoints for the ESP32 MMDVM Hotspot web interface.

**Base URL:** `http://<device-ip>/` or `http://esp32-mmdvm.local/`

**Authentication:** Most endpoints require HTTP Basic Authentication with configured web username/password.

---

## Web Pages (GET)

### Main Pages

#### `GET /`
**Description:** Home page with live DMR activity and recent transmission history
**Authentication:** Required
**Response:** HTML page

#### `GET /status`
**Description:** System status page with WiFi, DMR network, and MMDVM hardware status
**Authentication:** Required
**Response:** HTML page

#### `GET /serialmonitor`
**Description:** Real-time MMDVM serial communication monitor
**Authentication:** Required
**Response:** HTML page

#### `GET /wificonfig`
**Description:** WiFi configuration page for managing network connections
**Authentication:** Required
**Response:** HTML page

#### `GET /modeconfig`
**Description:** Mode configuration page for DMR settings, modem type, and protocols
**Authentication:** Required
**Response:** HTML page

#### `GET /admin`
**Description:** Administration page for system management and maintenance
**Authentication:** Required
**Response:** HTML page

---

## Data Endpoints (GET)

### System Status

#### `GET /statusdata`
**Description:** Retrieve current system status as HTML fragment
**Authentication:** Required
**Response:** HTML content with status cards
**Used by:** Auto-refresh on status page

#### `GET /logs`
**Description:** Retrieve serial log entries
**Authentication:** Required
**Response:** Plain text log entries

---

### DMR Activity

#### `GET /dmr-activity`
**Description:** Get combined live DMR activity for both slots
**Authentication:** Required
**Response:** HTML content with both slot activities
**Update Rate:** Real-time (recommended polling: 1-2 seconds)

#### `GET /dmr-slot1`
**Description:** Get live DMR activity for Slot 1 only
**Authentication:** Required
**Response:** HTML content with Slot 1 activity
**Note:** Only relevant for dual-slot modems

#### `GET /dmr-slot2`
**Description:** Get live DMR activity for Slot 2 only
**Authentication:** Required
**Response:** HTML content with Slot 2 activity

#### `GET /dmr-history`
**Description:** Get recent DMR transmission history (last 15 transmissions)
**Authentication:** Required
**Response:** HTML table with transmission history including:
- Timestamp
- Source callsign (linked to QRZ.com)
- Name, Location
- DMR ID
- Destination (TG/Private)
- Slot number
- Duration
- BER (Bit Error Rate)
- RSSI (Signal Strength)

---

### Configuration

#### `GET /wifiscan`
**Description:** Scan for available WiFi networks
**Authentication:** Required
**Response:** JSON array of WiFi networks
```json
[
  {
    "ssid": "NetworkName",
    "rssi": -45,
    "encryption": 3
  }
]
```

#### `GET /export-config`
**Description:** Export complete configuration as text file
**Authentication:** Required
**Response:** Plain text configuration file in INI format
**Sections:**
- `[DMR_CONFIG]` - DMR settings
- `[WIFI_CONFIG]` - WiFi networks
- `[SYSTEM_CONFIG]` - System settings including modem type
- `[MODE_CONFIG]` - Protocol modes

#### `GET /showprefs`
**Description:** Display all stored NVS preferences with values
**Authentication:** Required
**Response:** HTML page with categorized preferences:
- DMR Configuration (15 items)
- WiFi Networks (15 items)
- System Settings (5 items, including modem_type)
- Mode Configuration (6 items)
- Web Interface (2 items)

---

## Configuration Endpoints (POST)

### WiFi Configuration

#### `POST /saveconfig`
**Description:** Save WiFi network configurations (5 slots)
**Authentication:** Required
**Parameters:**
- `wifi0_label` - Label for WiFi slot 0
- `wifi0_ssid` - SSID for WiFi slot 0
- `wifi0_pass` - Password for WiFi slot 0
- (Same pattern for wifi1-wifi4)

**Response:** Redirect to `/wificonfig`

---

### DMR Configuration

#### `POST /savedmrconfig`
**Description:** Save DMR configuration settings
**Authentication:** Required
**Parameters:**

**General:**
- `callsign` - DMR callsign
- `dmr_id` - DMR ID (7 digits, 1000000-9999999)
- `rx_freq` - RX frequency in Hz
- `tx_freq` - TX frequency in Hz

**Network:**
- `server` - BrandMeister server address
- `password` - Hotspot password
- `essid` - ESSID suffix (0-99)
- `mode_dmr` - Enable/disable DMR mode (checkbox, value=1)

**Modem:**
- `modem_type` - Modem hardware type (see modem types below)
- `power` - Power level (0-99)
- `color_code` - Color code (0-15)

**Location:**
- `latitude` - Latitude (decimal degrees)
- `longitude` - Longitude (decimal degrees)
- `height` - Antenna height in meters
- `location` - Location description (max 20 chars)
- `description` - Station description (max 19 chars)
- `url` - Station URL (max 124 chars)

**Response:** HTML confirmation page, device restarts after 5 seconds

**Modem Types:**
- `mmdvmhshat` - MMDVM_HS_Hat (DB9MAT & DF2ET) for Pi (GPIO)
- `mmdvmhsdualhatgpio` - MMDVM_HS_Dual_Hat for Pi (GPIO)
- `mmdvmhshat12` - MMDVM_HS_Hat for Pi (GPIO 1.2)
- `mmdvmhsdualhat12` - MMDVM_HS_Dual_Hat for Pi (GPIO 1.2)
- `hs_hat_ambe` - HS_HAT with AMBE chip
- `hs_dual_hat_ambe` - HS_DUAL_HAT with AMBE chip
- `nano_hotspot` - Nano hotSPOT (BI7JTA)
- `nano_dv` - NanoDV (BI7JTA)
- `d2rg_mmdvm_hs` - D2RG MMDVM_HS RPi Hat
- `mmdvm_hs_dual_hat_14_7` - MMDVM_HS_Dual_Hat 14.7456 MHz
- `hs_hat_sky` - HS_HAT with SkyBridge chip

#### `POST /savemodes`
**Description:** Save mode enable/disable settings
**Authentication:** Required
**Parameters:**
- `mode_dmr` - Enable DMR mode (checkbox, value=1)

**Response:** HTML confirmation page, device restarts after 3 seconds

---

### System Configuration

#### `POST /save-hostname`
**Description:** Save mDNS hostname
**Authentication:** Required
**Parameters:**
- `hostname` - New hostname (alphanumeric and hyphens only)

**Response:** JSON `{"success": true}` or error message
**Note:** Device restarts to apply new hostname

#### `POST /save-verbose`
**Description:** Toggle verbose logging
**Authentication:** Required
**Parameters:**
- `verbose` - Enable verbose logging (checkbox, value=1)

**Response:** JSON `{"success": true}`

#### `POST /save-timezone`
**Description:** Save NTP timezone settings
**Authentication:** Required
**Parameters:**
- `timezone_offset` - Timezone offset in seconds
- `daylight_offset` - Daylight saving offset in seconds

**Response:** JSON `{"success": true}`

#### `POST /save-username`
**Description:** Change web interface username
**Authentication:** Required
**Parameters:**
- `username` - New username (min 3 characters)

**Response:** JSON `{"success": true}` or error message

#### `POST /save-password`
**Description:** Change web interface password
**Authentication:** Required
**Parameters:**
- `password` - New password (min 6 characters)

**Response:** JSON `{"success": true}` or error message

---

### Configuration Import

#### `POST /import-config`
**Description:** Import configuration from file
**Authentication:** Required
**Content-Type:** `multipart/form-data`
**Parameters:**
- File upload with configuration in INI format

**Response:** HTML confirmation page with import results
**Note:** Device restarts after import to apply settings

---

## System Actions (POST)

### Log Management

#### `POST /clearlogs`
**Description:** Clear serial monitor logs
**Authentication:** Required
**Response:** JSON `{"success": true}`

---

### System Control

#### `POST /reboot`
**Description:** Reboot the ESP32 device
**Authentication:** Required
**Response:** JSON `{"success": true, "message": "Rebooting..."}`
**Note:** Device restarts immediately

#### `POST /restart-services`
**Description:** Restart DMR network services without rebooting
**Authentication:** Required
**Response:** JSON `{"success": true, "message": "Services restarting..."}`

---

### Configuration Reset

#### `GET /resetconfig`
**Description:** Show configuration reset confirmation page
**Authentication:** Required
**Response:** HTML confirmation page with warning

#### `POST /confirmreset`
**Description:** Execute complete NVS storage wipe
**Authentication:** Required
**Response:** HTML confirmation page
**Note:** Wipes ALL stored preferences, device restarts to factory defaults

---

### Advanced Actions

#### `POST /test-mmdvm`
**Description:** Test MMDVM modem communication
**Authentication:** Required
**Response:** Plain text test results

#### `POST /cleanup-prefs`
**Description:** Clean up unused NVS preferences
**Authentication:** Required
**Response:** JSON with cleanup results

---

## Firmware Update

#### `POST /download-update`
**Description:** Download firmware from GitHub
**Authentication:** Required
**Parameters:**
- `channel` - Update channel: `stable` or `beta`

**Response:** JSON with download status and progress

#### `POST /upload-firmware`
**Description:** Upload firmware binary via web interface
**Authentication:** Required
**Content-Type:** `multipart/form-data`
**Response:** JSON with upload progress

#### `POST /flash-firmware`
**Description:** Flash uploaded firmware to device
**Authentication:** Required
**Response:** JSON with flash status
**Note:** Device restarts after successful flash

---

## Response Codes

- `200 OK` - Request successful
- `400 Bad Request` - Invalid parameters
- `401 Unauthorized` - Authentication required or failed
- `404 Not Found` - Endpoint does not exist
- `500 Internal Server Error` - Server error

---

## Notes

### Authentication
All endpoints require HTTP Basic Authentication except for:
- OTA update endpoints (use different authentication)

### Rate Limiting
- DMR activity endpoints: Poll every 1-2 seconds for real-time updates
- Status endpoints: Poll every 5 seconds (default auto-refresh)
- WiFi scan: Avoid frequent scans (takes 3-5 seconds)

### CORS
CORS is not enabled by default. Access from external domains requires same-origin policy.

### Content Types
- HTML pages: `text/html`
- JSON responses: `application/json`
- Config export: `text/plain`
- Firmware upload: `multipart/form-data`

---

## Example Usage

### curl Examples

**Get DMR Activity:**
```bash
curl -u admin:pi-star http://esp32-mmdvm.local/dmr-activity
```

**Export Configuration:**
```bash
curl -u admin:pi-star http://esp32-mmdvm.local/export-config -o config.txt
```

**Save Hostname:**
```bash
curl -u admin:pi-star -X POST http://esp32-mmdvm.local/save-hostname \
  -d "hostname=my-hotspot"
```

**Scan WiFi Networks:**
```bash
curl -u admin:pi-star http://esp32-mmdvm.local/wifiscan
```

**Reboot Device:**
```bash
curl -u admin:pi-star -X POST http://esp32-mmdvm.local/reboot
```

---

## JavaScript Fetch Examples

**Get DMR History:**
```javascript
fetch('/dmr-history', {
  credentials: 'include'
})
  .then(response => response.text())
  .then(html => {
    document.getElementById('history').innerHTML = html;
  });
```

**Save Verbose Logging:**
```javascript
fetch('/save-verbose', {
  method: 'POST',
  headers: {
    'Content-Type': 'application/x-www-form-urlencoded',
  },
  body: 'verbose=1',
  credentials: 'include'
})
  .then(response => response.json())
  .then(data => console.log(data));
```

---

## Version

**API Version:** 1.0
**Firmware Version:** 20251204_ESP32_BETA
**Last Updated:** December 5, 2025
