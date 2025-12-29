# MQTT Integration for ESP32 MMDVM Hotspot

This document describes the MQTT pub/sub functionality available in the ESP32 MMDVM Hotspot firmware.

## Overview

MQTT (Message Queuing Telemetry Transport) is a lightweight publish/subscribe messaging protocol designed for IoT devices. This hotspot implementation publishes real-time status information and DMR activity to an MQTT broker, enabling integration with:

- Home automation systems (Home Assistant, OpenHAB, etc.)
- Monitoring dashboards (Node-RED, Grafana, etc.)
- Custom applications and scripts
- Mobile apps
- Logging systems

## Configuration

### Web Interface

Configure MQTT settings via the Admin page in the web interface:

1. Navigate to the Admin page
2. Find the "MQTT Configuration" card
3. Configure the following settings:
   - **Enable MQTT**: Toggle to enable/disable MQTT functionality
   - **Broker**: MQTT broker hostname or IP address (e.g., `192.168.1.100` or `broker.hivemq.com`)
   - **Port**: MQTT broker port (default: 1883, secure: 8883)
   - **Username**: MQTT username (leave empty if no authentication required)
   - **Password**: MQTT password (leave empty if no authentication required)
   - **Client ID**: MQTT client identifier (default: uses your DMR callsign)
   - **Topic Prefix**: Base topic prefix (default: `{hostname}/{callsign}`, e.g., `esp32-mmdvm/PD2EMC`)
   - **Publish Interval**: How often to publish status updates in milliseconds (5000-300000 ms, default: 30000 ms / 30 seconds)

### Configuration File

Default MQTT settings can be defined in `config.h`:

```cpp
// ===== MQTT Configuration =====
#define MQTT_ENABLED false                    // Enable/disable MQTT
#define MQTT_BROKER ""                        // MQTT broker hostname or IP
#define MQTT_PORT 1883                        // MQTT broker port
#define MQTT_USERNAME ""                      // MQTT username (empty = no auth)
#define MQTT_PASSWORD ""                      // MQTT password (empty = no auth)
#define MQTT_CLIENT_ID ""                     // MQTT client ID (default: DMR callsign)
#define MQTT_TOPIC_PREFIX ""                  // Topic prefix (default: {hostname}/{callsign})
#define MQTT_PUBLISH_INTERVAL 30000           // Publish interval in milliseconds
```

## MQTT Topics

All topics use the configured prefix (default: `{hostname}/{callsign}`). For example, if your hostname is `esp32-mmdvm` and callsign is `PD2EMC`, the prefix would be `esp32-mmdvm/PD2EMC`.

### Status Topics (Published Periodically)

These topics are published every 30 seconds (or according to your configured interval) and immediately upon MQTT connection:

#### `{prefix}/system/status`

Detailed system information including uptime, chip details, memory, flash, and firmware.

**Example payload:**
```json
{
  "callsign": "PD2EMC",
  "dmr_id": 2041152,
  "hostname": "esp32-mmdvm",
  "uptime": {
    "seconds": 2758,
    "days": 0,
    "hours": 0,
    "minutes": 45,
    "secondsRemaining": 58
  },
  "chip": {
    "model": "ESP32-S3",
    "revision": 2,
    "cores": 2,
    "cpuFreqMHz": 240
  },
  "memory": {
    "freeHeapKB": 192.9,
    "freeHeapPercent": 63,
    "minFreeHeapKB": 147.1,
    "heapSizeKB": 303.5,
    "psramSizeMB": 8,
    "freePsramKB": 8156.8
  },
  "flash": {
    "sizeMB": 16,
    "speedMHz": 80,
    "sketchSizeKB": 1424.9,
    "freeSketchSpaceKB": 3072
  },
  "firmware": {
    "sdkVersion": "v5.5.1-931-g9bb7aa84fe",
    "version": "20251229_ESP32_BETA",
    "buildDate": "Dec 29 2025 12:50:02"
  }
}
```

#### `{prefix}/modem/status`

Modem hardware and firmware information with parsed details.

**Example payload:**
```json
{
  "ready": true,
  "type": "single",
  "hardware": "MMDVM_HS_Hat",
  "firmwareVersion": "1.6.1",
  "buildDate": "2023-11-15",
  "crystal": "14.7456MHz",
  "transceiver": "ADF7021",
  "author": "CA6JAU, G4KLX, W0CHP.",
  "gitId": "#7e16099"
}
```

#### `{prefix}/network/status`

DMR network connection status.

**Example payload:**
```json
{
  "dmr_logged_in": true,
  "dmr_server": "api.brandmeister.network",
  "status": "Connected",
  "talkgroup": 91
}
```

### Activity Topics (Event-Driven)

These topics are published in real-time when DMR activity occurs:

#### `{prefix}/slot1/activity`
#### `{prefix}/slot2/activity`

Real-time DMR transmission information for each slot. Published when a new transmission starts.

**Example payload (active transmission):**
```json
{
  "slot": 2,
  "active": true,
  "src_id": 123456,
  "dst_id": 91,
  "callsign": "CALLSIGN",
  "name": "Name",
  "city": "City",
  "country": "Country",
  "call_type": "group",
  "duration": 4,
  "timestamp": 2198
}
```

**Field descriptions:**
- `slot`: Slot number (1 or 2)
- `active`: Always `true` (only published during active transmissions)
- `src_id`: Source DMR ID
- `dst_id`: Destination DMR ID (talkgroup for group calls)
- `callsign`: Source station callsign
- `name`: Operator name (if available from RadioID.net)
- `city`: Operator city (if available)
- `country`: Operator country (if available)
- `call_type`: "group" for talkgroup calls, "private" for private calls
- `duration`: Duration in seconds since transmission started
- `timestamp`: Unix timestamp (seconds since boot / 1000)

**Note:** When a transmission ends, messages stop being published for that slot. Subscribers should implement a timeout (e.g., 5 seconds) to detect when transmissions end.

### Availability Topic (Last Will Testament)

#### `{prefix}/availability`

Online/offline status using MQTT Last Will Testament (LWT). This topic has the "retained" flag set, so new subscribers immediately see the current state.

**Possible values:**
- `online` - Hotspot is connected to MQTT broker
- `offline` - Hotspot disconnected (published automatically by broker via LWT)

## Example Use Cases

### Home Assistant Integration

Monitor your hotspot status in Home Assistant:

```yaml
mqtt:
  sensor:
    - name: "Hotspot Status"
      state_topic: "esp32-mmdvm/PD2EMC/availability"

    - name: "Hotspot Uptime"
      state_topic: "esp32-mmdvm/PD2EMC/system/status"
      value_template: "{{ value_json.uptime.seconds }}"
      unit_of_measurement: "s"

    - name: "Current Talkgroup"
      state_topic: "esp32-mmdvm/PD2EMC/network/status"
      value_template: "{{ value_json.talkgroup }}"

    - name: "Slot 2 Activity"
      state_topic: "esp32-mmdvm/PD2EMC/slot2/activity"
      value_template: "{{ value_json.callsign }}"
```

### Node-RED Dashboard

Create a real-time dashboard showing:
- System status and uptime
- Active transmissions on both slots
- Last heard stations
- Network connectivity

### Python Script Example

Monitor slot activity:

```python
import paho.mqtt.client as mqtt
import json

def on_message(client, userdata, message):
    topic = message.topic
    payload = json.loads(message.payload)

    if '/slot' in topic:
        if payload['active']:
            print(f"Slot {payload['slot']}: {payload['callsign']} "
                  f"({payload.get('name', 'Unknown')}) -> TG{payload['dst_id']} "
                  f"Duration: {payload['duration']}s")

client = mqtt.Client()
client.on_message = on_message
client.connect("192.168.1.100", 1883)
client.subscribe("esp32-mmdvm/PD2EMC/slot#")
client.loop_forever()
```

## Technical Details

### Connection Behavior

- **Automatic Reconnection**: If connection to broker is lost, the hotspot automatically attempts to reconnect every 5 seconds
- **Immediate Publishing**: Upon successful connection, the hotspot immediately publishes system/status, modem/status, and network/status
- **Buffer Size**: MQTT message buffer is set to 1024 bytes to accommodate the enhanced payloads

### Message Retention

- `availability` topic uses the retained flag, so new subscribers immediately see online/offline status
- Other topics are not retained to avoid publishing stale data

### Quality of Service (QoS)

All messages are published with QoS 0 (at most once delivery) for optimal performance.

## Troubleshooting

### Not Receiving Messages

1. **Check MQTT is enabled** in the Admin page
2. **Verify broker connection** - Check the `/api/mqtt-monitor` endpoint for connection status
3. **Check topic subscription** - Ensure you're subscribed to the correct topic with your configured prefix
4. **Firewall/Network** - Ensure MQTT port (usually 1883) is accessible

### Messages Too Large

If you experience issues with large payloads:
- The buffer size is set to 1024 bytes
- System/status payload is the largest (~600-800 bytes)
- If using a constrained broker, consider filtering or limiting the data you need

### Authentication Issues

- Ensure username and password are correct
- Some brokers require specific client ID formats
- Check broker logs for authentication errors

## Monitoring MQTT Status

The hotspot provides a JSON API endpoint for monitoring MQTT status:

**Endpoint:** `/api/mqtt-monitor`

**Example response:**
```json
{
  "enabled": true,
  "connected": true,
  "broker": "192.168.1.100",
  "port": 1883,
  "client_id": "PD2EMC",
  "topic_prefix": "esp32-mmdvm/PD2EMC",
  "publish_interval": 30000,
  "last_publish": 12345,
  "connect_attempts": 3,
  "mqtt_state": 0,
  "state_description": "Connected",
  "topics": {
    "system": "esp32-mmdvm/PD2EMC/system/status",
    "modem": "esp32-mmdvm/PD2EMC/modem/status",
    "network": "esp32-mmdvm/PD2EMC/network/status",
    "slot1": "esp32-mmdvm/PD2EMC/slot1/activity",
    "slot2": "esp32-mmdvm/PD2EMC/slot2/activity",
    "availability": "esp32-mmdvm/PD2EMC/availability"
  }
}
```

## Security Considerations

- **Use authentication** - Always configure username/password for production brokers
- **Network security** - Consider using MQTT over TLS (port 8883) for sensitive deployments
- **Access control** - Configure your MQTT broker with appropriate ACLs to restrict topic access
- **Password storage** - MQTT credentials are stored in ESP32 NVS (encrypted storage)

## Support

For issues, feature requests, or questions:
- GitHub: https://github.com/javastraat/esp32_mmdvm_hotspot
- Check the main README.md for general project information
