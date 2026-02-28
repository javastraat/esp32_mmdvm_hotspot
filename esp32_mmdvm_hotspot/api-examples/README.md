# ESP32 MMDVM Hotspot - API Examples & Documentation

This directory contains ready-to-use Node-RED flows and complete API documentation for monitoring and integrating with your ESP32 MMDVM Hotspot.

##  Directory Structure

```
api-examples/
├── README.md                                   # This file
├── node-red/
│   ├── 01-basic-api-calls.json                # Basic API testing flow
│   ├── 02-monitoring-dashboard.json           # Full dashboard with UI
│   └── 03-advanced-examples.json              # Advanced integrations
└── openapi/
    └── swagger.json                            # OpenAPI 3.0 specification
```

##  What's Included

### Node-RED Flows

#### 1. **01-basic-api-calls.json**
Simple HTTP requests to all API endpoints - perfect for getting started and testing.

**Features:**
- One-click test for each endpoint
- View JSON responses in debug panel
- No additional nodes required

**Use this for:**
- Testing your API configuration
- Learning the API response formats
- Building custom integrations

---

#### 2. **02-monitoring-dashboard.json**
Complete monitoring dashboard with real-time charts and gauges.

**Features:**
- Auto-refreshing dashboard (5-second intervals)
- Memory usage gauge with threshold alerts
- Memory usage chart (10-minute history)
- System uptime display
- DMR activity monitoring (both slots)
- Network and modem status indicators

**Prerequisites:**
```bash
cd ~/.node-red
npm install node-red-dashboard
```

**Access dashboard at:** `http://your-node-red-ip:1880/ui`

---

#### 3. **03-advanced-examples.json**
Advanced integration examples including alerts, logging, and notifications.

**Features:**
- **Memory Alert System** - Sends alerts when memory drops below 20%
- **Callsign Watch** - Notifications when specific callsigns are active
- **DMR History Logger** - Save transmissions to CSV or database
- **Database Integration** - InfluxDB example for time-series data
- **Telegram Bot** - Query status via Telegram commands

**Optional Prerequisites:**
```bash
# For Telegram notifications
npm install node-red-contrib-telegrambot

# For InfluxDB logging
npm install node-red-contrib-influxdb

# For MySQL logging
npm install node-red-node-mysql
```

---

### OpenAPI Specification

#### **openapi/swagger.json**
Complete OpenAPI 3.0 specification for the ESP32 MMDVM Hotspot API.

**Features:**
- Full API documentation in machine-readable format
- Compatible with Swagger UI, Postman, and other API tools
- Includes request/response schemas, authentication details, and examples

**Use this for:**
- Importing into Postman or Insomnia for API testing
- Generating API client code in various languages
- Setting up Swagger UI for interactive API documentation
- API validation and contract testing

**Quick Start with Swagger UI:**
1. Visit [Swagger Editor](https://editor.swagger.io/)
2. File → Import File → Choose `openapi/swagger.json`
3. View interactive documentation and test endpoints directly

---

##  Quick Start (Node-RED)

### Step 1: Import the Flow

1. Open Node-RED in your browser
2. Click the **menu** (☰) in the top right
3. Select **Import**
4. Click **"select a file to import"**
5. Choose one of the JSON files from the `node-red/` directory
6. Click **Import**

### Step 2: Configure Authentication

1. Double-click any **HTTP Request** node (blue node)
2. Click the **pencil icon** next to "Use authentication"
3. Select **"Type: basic authentication"**
4. Enter your ESP32 web username and password
5. Click **Update** then **Done**

### Step 3: Set Your ESP32 Address

**For 01-basic-api-calls.json:**
- Edit each HTTP Request node
- Set the **URL** to your ESP32 address
- Examples: `http://esp32-mmdvm.local` or `http://192.168.1.100`

**For 02-monitoring-dashboard.json & 03-advanced-examples.json:**
- Find the **"Set Base URL"** change node (yellow node)
- Edit and update `msg.baseUrl` to your ESP32 address

### Step 4: Deploy

1. Click the **Deploy** button (top right)
2. Test by clicking an inject button (blue square on left of inject nodes)
3. Check the **debug panel** (bug icon on right sidebar) for responses

---

##  Dashboard Screenshots

The monitoring dashboard (`02-monitoring-dashboard.json`) displays:

- **System Status** panel:
  - Memory usage gauge (0-100%)
  - Memory usage chart over time
  - Uptime display
  - CPU frequency
  - Firmware version
  - WiFi connection info

- **DMR Activity** panel:
  - Server connection status
  - MMDVM ready indicator (LED)
  - Slot 1 activity (callsign, name, talkgroup, duration)
  - Slot 2 activity

- **Modem Info** panel:
  - Hardware model
  - Firmware version
  - Transceiver type and crystal frequency

---

##  Customization Tips

### Change Refresh Intervals

Edit the **inject** nodes:
- Basic calls: Manual trigger (click to test)
- Dashboard: Every 5 seconds (change "Repeat" field)
- Advanced: Various intervals (30s for memory, 2s for DMR activity)

### Modify Alert Thresholds

In `03-advanced-examples.json`, edit the function nodes:
```javascript
const threshold = 20; // Change to your desired percentage
```

### Add Custom Callsigns to Watch

In the **"Check Watchlist"** function node:
```javascript
const watchlist = ['PD2EMC', 'N0CALL', 'W1AW']; // Add your callsigns
```

### Database Configuration

For InfluxDB, add an **InfluxDB Out** node:
1. Drag from palette on left
2. Configure connection (host, database, username, password)
3. Connect to the **"Format for InfluxDB"** function output

---

##  Use Cases

### Home Automation Integration
- Trigger Home Assistant scenes when specific callsigns are active
- Control lights/indicators based on DMR activity
- Log activity to Home Assistant database

### External Dashboards
- Feed data to Grafana for beautiful visualizations
- Create custom dashboards with metrics over time
- Set up alerting based on thresholds

### Notifications
- Telegram bot for remote status queries
- Email alerts for low memory or disconnect events
- SMS notifications via Twilio integration

### Data Analysis
- Export to CSV for offline analysis
- Store in database for long-term trends
- Generate daily/weekly reports

---

##  API Endpoints Reference

All endpoints use HTTP Basic Authentication.

| Endpoint | Description | Refresh Recommendation |
|----------|-------------|----------------------|
| `/api/status` | Network & DMR status | Every 5-10 seconds |
| `/api/system-information` | ESP32 hardware details | Every 30-60 seconds |
| `/api/modem-information` | MMDVM modem info | Once on startup |
| `/api/dmr-activity` | Live DMR activity (both slots) | Every 1-2 seconds |
| `/api/dmr-slot1` | DMR Slot 1 only | Every 1-2 seconds |
| `/api/dmr-slot2` | DMR Slot 2 only | Every 1-2 seconds |
| `/api/dmr-history` | Recent DMR transmissions (15 max) | Every 10-30 seconds |
| `/api/rf-history` | Local RF activity (15 max) | Every 10-30 seconds |
| `/api/system-status` | System status overview | Every 5-10 seconds |
| `/api/logs` | Serial monitor logs (50 max) | Every 5-10 seconds |
| `/api/wifiscan` | Scan WiFi networks | On demand only |

**Note:** Avoid polling too frequently to prevent overloading the ESP32.

---

##  Troubleshooting

### "Error: connect EHOSTUNREACH"
- Check that your ESP32 is powered on and connected to network
- Verify the IP address or hostname is correct
- Try pinging the device: `ping esp32-mmdvm.local`

### "Error: 401 Unauthorized"
- Double-check your username and password
- Make sure authentication is configured in HTTP Request nodes

### "Error: getaddrinfo ENOTFOUND"
- mDNS hostname not resolving
- Try using the IP address directly instead of `.local` hostname
- Check that Node-RED and ESP32 are on the same network

### Dashboard Not Showing
- Make sure node-red-dashboard is installed
- Restart Node-RED after installing
- Access dashboard at `http://your-node-red-ip:1880/ui`

### No Data Appearing
- Check debug panel for errors
- Verify inject nodes are triggering (look for timestamps)
- Deploy the flow after making changes

---

##  Additional Ideas

Here are more integration ideas you can build:

1. **Discord Bot** - Post DMR activity to Discord channel
2. **MQTT Publisher** - Publish data to MQTT broker for IoT integration
3. **Voice Announcements** - Use TTS to announce active callsigns
4. **LED Strip Control** - Change colors based on activity
5. **QRZ.com Integration** - Fetch and display operator info
6. **Mapping** - Plot operator locations on a map
7. **Statistics** - Track most active callsigns, talkgroups, etc.
8. **Web Dashboard** - Build custom web UI with WebSockets
9. **Mobile App** - Use Node-RED with mobile dashboard
10. **Automatic Logging** - Daily exports to cloud storage

---

##  Documentation

For complete API documentation, see:
- [`../API_README.md`](../API_README.md) - Full API documentation with examples
- [`openapi/swagger.json`](openapi/swagger.json) - OpenAPI 3.0 specification

---

##  Contributing

Have a cool Node-RED flow to share? Submit a pull request!

Ideas for new examples:
- Multi-hotspot monitoring (multiple ESP32 devices)
- APRS-IS integration
- Audio streaming integration
- Custom alerting rules
- Advanced analytics and reporting

---

##  License

These Node-RED flows are provided as examples and are free to use and modify for your needs.

---

**Happy Monitoring! 73 de PD2EMC** 
