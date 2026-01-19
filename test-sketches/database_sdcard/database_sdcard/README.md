# ESP32 DMR Database Lookup

This Arduino sketch provides a web server on ESP32 for searching DMR radio user databases stored on an SD card. It supports both CSV file lookups and SQLite database queries.

## Features

- Web interface for searching DMR users
- REST API for programmatic access
- CSV file search (basic lookup by Radio ID)
- SQLite database search (advanced search with multiple fields)
- Wildcard search support using `*`
- Automatic database download from remote servers
- SD card storage for database files

## Hardware Requirements

- ESP32 development board
- SD card module
- SD card (FAT32 formatted)

## API Endpoints

### CSV Lookup

#### GET /api/dmr/user/?id={radio_id}

Look up a DMR user by Radio ID from the CSV file.

**Parameters:**
- `id` - The DMR Radio ID to search for

**Example:**
```
GET /api/dmr/user/?id=2041126
```

**Response:**
```json
{
  "radio_id": 123456,
  "callsign": "N0CALL",
  "name": "John",
  "city": "Amsterdam",
  "state": "NH",
  "country": "Netherlands"
}
```

#### GET /api/dmr/status

Get the current search status (for async searches).

**Response:**
```json
{
  "searching": false,
  "progress": 100,
  "result": { ... }
}
```

### SQLite Database Search

#### GET /api/sqlite/search?field={field}&value={value}

Search the SQLite database by various fields.

**Parameters:**
- `field` - The field to search by. Options:
  - `radio_id` - DMR Radio ID
  - `callsign` - Amateur radio callsign
  - `name` - First name
  - `city` - City name
  - `state` - State/province
  - `country` - Country name
- `value` - The search value (supports `*` wildcard)

**Wildcard Search:**
Use `*` as a wildcard character:
- `PD*` - Matches callsigns starting with "PD"
- `*EMC` - Matches callsigns ending with "EMC"
- `*2E*` - Matches callsigns containing "2E"
- `204*` - Matches radio IDs starting with "204"

**Examples:**
```
GET /api/sqlite/search?field=callsign&value=PD2EMC
GET /api/sqlite/search?field=callsign&value=PD*
GET /api/sqlite/search?field=city&value=Amsterdam
GET /api/sqlite/search?field=country&value=Nether*
GET /api/sqlite/search?field=radio_id&value=204*
```

**Response:**
```json
{
  "results": [
    {
      "radio_id": 123456,
      "callsign": "N0CALL",
      "first_name": "John",
      "city": "Amsterdam",
      "state": "NH",
      "country": "Netherlands"
    },
    ...
  ]
}
```

**Notes:**
- Results are limited to 100 records (except exact radio_id match which returns 1)
- Searches are case-insensitive
- Name, city, state, and country fields use partial matching by default (no wildcard needed)

### Management Endpoints

| Endpoint | Description |
|----------|-------------|
| `GET /` | Web interface |
| `GET /download` | Start CSV database download |
| `GET /download_db` | Start SQLite database download |
| `GET /status` | CSV download progress |
| `GET /status_db` | SQLite download progress |
| `GET /delete` | Delete CSV database |
| `GET /delete_db` | Delete SQLite database |
| `GET /check` | Check for database updates |

## Database Schema

The radioid table has the following columns:

| Column | Type | Description |
|--------|------|-------------|
| RADIO_ID | INTEGER | DMR Radio ID (primary key) |
| CALLSIGN | TEXT | Amateur radio callsign |
| FIRST_NAME | TEXT | User's first name |
| CITY | TEXT | City |
| STATE | TEXT | State/Province |
| COUNTRY | TEXT | Country |

## Performance

SQLite searches are optimized with indexes on all searchable columns using `COLLATE NOCASE` for case-insensitive queries.

Typical search times on ESP32 with SD card:
- Exact match: ~250ms
- Wildcard search: ~250-500ms (depending on result count)

## Configuration

Edit these constants in the sketch:

```cpp
const char* ssid     = "YourWiFi";
const char* password = "YourPassword";

const char* destFile_db = "/database/esp32_database.db";
const char* fileURL_db = "http://yourserver/database.db";
```

## Building the Database

Use the `database-app-tool.py` script to create the SQLite database from JSON files:

```bash
# Import data with indexes
python database-app-tool.py -i radioid radioid.json --index

# List tables
python database-app-tool.py -t

# Show table structure
python database-app-tool.py -s radioid
```

## License

Part of the ESP32 MMDVM Hotspot project.
