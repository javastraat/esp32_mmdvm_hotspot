# Node-RED Integration Flows

This directory contains ready-to-import Node-RED flows for monitoring and integrating with your ESP32 MMDVM Hotspot.

## Available Flows

### 01-basic-api-calls.json
**Purpose:** Simple API testing and exploration

**What it does:**
- Manual trigger buttons for each API endpoint
- Debug output to view JSON responses
- No additional Node-RED nodes required

**Best for:** Learning the API, testing connectivity, building custom integrations

---

### 02-monitoring-dashboard.json
**Purpose:** Complete monitoring dashboard with real-time UI

**What it does:**
- Auto-refreshing dashboard (5-second intervals)
- Memory usage gauge and historical chart
- DMR activity display for both slots
- System status indicators
- Modem information panel

**Prerequisites:**
```bash
cd ~/.node-red
npm install node-red-dashboard
```

**Access:** `http://your-node-red-ip:1880/ui`

**Best for:** Real-time monitoring, quick status overview, NOC displays

---

### 03-advanced-examples.json
**Purpose:** Advanced integration patterns

**What it does:**
- Memory threshold alerts with cooldown
- Callsign watchlist notifications
- DMR history logging (CSV/database ready)
- InfluxDB integration example
- Telegram bot skeleton

**Optional Prerequisites:**
```bash
npm install node-red-contrib-telegrambot  # For Telegram
npm install node-red-contrib-influxdb     # For InfluxDB
npm install node-red-node-mysql           # For MySQL
```

**Best for:** Home automation, alerting, data logging, custom notifications

---

## Quick Start

1. **Import:** Menu (☰) → Import → Select file → Import
2. **Configure Authentication:**
   - Edit any HTTP Request node
   - Add Basic Authentication with your ESP32 credentials
3. **Set Base URL:**
   - Basic flow: Edit each HTTP Request node
   - Dashboard/Advanced: Edit the "Set Base URL" change node
4. **Deploy:** Click Deploy button
5. **Test:** Click inject nodes or view dashboard

---

## Documentation

For detailed instructions, see [../README.md](../README.md)

For API documentation, see:
- [../../API_README.md](../../API_README.md) - Full API reference
- [../openapi/swagger.json](../openapi/swagger.json) - OpenAPI spec

---

**73 de PD2EMC** 
