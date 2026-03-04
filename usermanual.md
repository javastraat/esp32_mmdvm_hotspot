# ESP32 MMDVM Hotspot User Manual
# ESP32 MMDVM Hotspot — Comprehensive User Manual

## Introduction
This manual provides a complete guide to the ESP32 MMDVM Hotspot, including hardware setup, firmware installation, and a detailed walkthrough of every web interface page and dashboard card. It is designed for new and advanced users alike.

---

## Hardware Requirements & Setup
- **ESP32** (dual-core; tested on ESP32 and ESP32-S3)
- **MMDVM HAT** or compatible modem (UART)
- *(Optional)* W5500 Ethernet module (SPI)
- *(Optional)* SSD1306/SH1106 OLED display (I2C, 128×64)
- *(Optional)* MicroSD card module (SPI)

### Default Pin Configuration
| Function      | Pin | Notes                |
## Introduction
This manual provides a comprehensive guide for setting up, configuring, and operating the ESP32 MMDVM Hotspot. It covers hardware requirements, firmware installation, web interface usage, configuration management, and advanced features for amateur radio operators.

---

## Hardware Requirements
- **ESP32** (dual-core; tested on ESP32 and ESP32-S3)
- **MMDVM HAT** or compatible modem (UART)
- *(Optional)* W5500 Ethernet module (SPI)
- *(Optional)* SSD1306/SH1106 OLED display (I2C, 128×64)
- *(Optional)* MicroSD card module (SPI)

### Default Pin Configuration
| Function      | Pin | Notes                |
|-------------- |-----|---------------------|
| MMDVM TX      | 43  | UART to modem       |
| MMDVM RX      | 44  | UART from modem     |
| MMDVM Boot    | 4   | Bootloader trigger  |

All pins are configurable via the web interface.

---

## Firmware Installation
1. Flash the firmware to your ESP32 using your preferred tool.
2. Power on the device. It will start in WiFi AP mode (`MMDVM-Setup`, password: `mmdvm1234`).
3. Connect to the AP and open `http://192.168.4.1` in your browser.
4. Log in with default credentials: `admin` / `pi-star`.

---

## Web Interface: Pages & Cards
The web UI is accessible from any browser on the local network. It provides configuration, monitoring, and management features. Below is a detailed explanation of every page and dashboard card.

### Main Pages

| Page         | URL                  | Description                                      |
| MMDVM Reset   | 13  | Hard reset          |
| OLED SDA      | 17  | I2C data            |
| OLED SCL      | 18  | I2C clock           |
| LED           | 38  | Status indicator    |
| Button        | 0   | OLED toggle         |
| SD MISO       | 9   | SPI                 |
| SD MOSI       | 11  | SPI                 |
| SD SCLK       | 10  | SPI                 |
| SD CS         | 12  | SPI                 |
| ETH MISO      | 47  | W5500 SPI           |
| ETH MOSI      | 21  | W5500 SPI           |
| ETH SCLK      | 48  | W5500 SPI           |
| ETH CS        | 45  | W5500 SPI           |
| ETH INT       | 14  | W5500 interrupt     |

All pins are configurable via the web interface.

---

## Firmware Installation

---

### Dashboard Cards: Detailed Explanations

#### Home Page Cards
- **On Air Status:** Shows current transmission state, active mode, and last heard station.
- **Last 15 Calls:** List of recent DMR calls with callsign, name, city, country, and time.
- **DAPNET Messages:** Recent paging messages received via DAPNET.
- **POCSAG Queue:** Current queue of POCSAG messages waiting for transmission.

#### System Status Cards
- **Station Information:** Callsign, DMR ID, SSID, and operational status.
- **WiFi Status:** Current connection, signal strength, IP address, and slot info.
- **Ethernet Status:** Link state, IP address, and hardware info (if enabled).
- **MMDVM Hardware Status:** Modem connection, firmware version, UART health.
- **MQTT Status:** Broker connection, last publish/subscribe, command token state.
- **WireGuard Status:** VPN tunnel state, endpoint, allowed IPs, DNS.
- **System Health:** ESP32 temperature, heap usage, PSRAM, per-task stack.

#### DMR Page Cards
- **DMR Activity:** Current call, last 15 calls, talker alias, and call history.

#### POCSAG/DAPNET Page Cards
- **POCSAG Queue:** Messages queued for RF transmission, with RIC and type.
- **DAPNET History:** Last 15 messages received from DAPNET network.

#### WiFi Page Cards
- **WiFi Status:** List of available networks, current slot, AP settings.

#### Firmware Page Cards
- **Firmware Status:** Current ESP32 and modem firmware versions, OTA partition info, update progress.

#### Admin Page Cards
- **Log Viewer:** Circular buffer of last 50 log messages, with timestamps and severity.
- **System Actions:** Buttons for factory reset, reboot, restart services, clear logs.

#### MQTT Page Cards
- **MQTT Status:** Broker connection, last publish/subscribe, command token.

#### WireGuard Page Cards
- **WireGuard Status:** Tunnel state, endpoint, allowed IPs, DNS.

#### SD Card Page Cards
- **SD Card Status:** Mounted state, total/free space, filesystem info.
- **File Browser:** Browse, upload, download, and delete files on SD card.

#### Serial Monitor Page Cards
- **Serial Status:** UART connection, baud rate, RX/TX pins, modem health.

#### System Info Page Cards
- **System Info:** ESP32 model, memory usage, partition info, task stack usage.

#### RF Settings Page Cards
- **RF Settings:** RX/TX frequency, color code, RF power, CW ID configuration.

---

## Configuration Management
- **Export/Import:** Download/upload all settings as a `key=value` text file.
- **Snapshots:** Save/load named configuration snapshots to SD card or LittleFS.
- **Factory Reset:** Erase all settings and reboot to defaults.

---

## MQTT Integration
- **Publish:** System status, logs, hardware info, DMR activity, POCSAG events.
- **Subscribe:** Command topic (token required).
- **Commands:**
  - `reboot`: Restart ESP32
  - `get_hardware`: Publish hardware info
  - `get_status`: Publish online status

---



## Default Credentials
| Setting            | Default         |
1. Flash the firmware to your ESP32 using your preferred tool.
3. Connect to the AP and open `http://192.168.4.1` in your browser.
4. Log in with default credentials: `admin` / `pi-star`.
## Web Interface Overview
---
- **Firmware update fails:** Use correct image/channel and ensure stable power.
---
## License
The web UI is accessible from any browser on the local network. It provides configuration, monitoring, and management features.
| Mode Select  | `/mode-select`       | Enable/disable radio protocols                   |
| POCSAG/DAPNET| `/mode-pocsag`       | Paging, DAPNET settings                          |
| MQTT         | `/system-mqtt`       | MQTT broker, topics, command token               |
| SD Card      | `/system-sdcard`     | SD card status, database, file browser           |
| RF Settings  | `/settings-mmdvm`    | RF frequency, color code, power, CW ID           |
### Dashboard Cards
- DAPNET Messages
- System Hardware, Memory, Task Stack Usage
- **Factory Reset:** Erase all settings and reboot to defaults.
---
- **Publish:** System status, logs, hardware info, DMR activity, POCSAG events.
- **Commands:**
  - `reboot`: Restart ESP32
  - `get_hardware`: Publish hardware info
  - `get_status`: Publish online status

---

## OTA Firmware Updates
- **ESP32 OTA:** Download and flash stable/beta/factory images from GitHub.
- **MMDVM Modem OTA:** UART bootloader flash from URL or direct upload.
- **ArduinoOTA:** Optional network upload from Arduino IDE.
- **Partition Switch:** Manual switch via web UI.

---

## Advanced Features
- **WireGuard VPN:** Secure tunnel, configurable endpoint, allowed IPs, DNS.
- **SD Card Database:** DMR user database in CSV/SQLite formats.
- **OLED Display:** Live call info, Talker Alias, async database lookup results.
- **NVS Viewer/Repair:** Inspect and repair NVS keys.
- **Monitoring:** Chip temperature, heap usage, PSRAM, per-task stack.

---

## Default Credentials
| Setting            | Default         |
|--------------------|----------------|
| Web username       | `admin`        |
| Web password       | `pi-star`      |
| WiFi AP SSID       | `MMDVM-Setup`  |
| WiFi AP password   | `mmdvm1234`    |
| ArduinoOTA password| `mmdvm`        |
| DMR callsign       | `N0CALL`       |
| DMR ID             | `1234567`      |

---

## Troubleshooting
- **Cannot connect to AP:** Check ESP32 power and reset. Ensure device is in AP mode.
- **Web UI not loading:** Confirm browser is connected to AP and using correct IP.
- **No RF output:** Verify modem wiring and enable mode in web UI.
- **SD card not detected:** Check wiring and format card as FAT32.
- **Firmware update fails:** Use correct image/channel and ensure stable power.

---

## License

**Amateur Radio Non-Commercial License**

This project is open source for amateur radio use only. 

**You are free to:**
- Use the software for amateur radio operations
- Study, modify, and improve the code
- Share and distribute modifications
- Contribute improvements back to the project

**Under the following conditions:**
- **Non-Commercial:** You may NOT use this software for commercial purposes
- **Amateur Radio Only:** This software is intended exclusively for licensed amateur radio operators
- **Attribution:** You must give appropriate credit to the original authors (PD2EMC & PD8JO)
- **Share Alike:** If you modify and distribute this software, you must use the same license

**Specifically prohibited:**
- Commercial sale of this software or derivatives
- Commercial hardware products using this software without explicit permission
- Use by unlicensed individuals for radio transmission
- Any commercial exploitation of the codebase

**Legal Requirements:**
- Valid amateur radio license required for operation
- Compliance with local radio regulations mandatory
- Proper station identification required per your jurisdiction

For commercial licensing inquiries, contact the authors.

## Resources and Documentation

### Official Resources
- **MMDVM Project:** https://github.com/g4klx/MMDVM
- **MMDVMHost:** https://github.com/g4klx/MMDVMHost  
- **BrandMeister Network:** https://brandmeister.network/
- **Pi-Star:** https://www.pistar.uk/
- **ESP32 Arduino:** https://github.com/espressif/arduino-esp32

### Hardware Vendors
- **JumboSPOT:** https://www.amateurwireless.com/
- **MMDVM_HS:** https://github.com/juribeparada/MMDVM_HS
- **ZUMspot:** https://www.zumspot.com/

### DMR Resources  
- **RadioID.net Database:** https://radioid.net/
- **BrandMeister Dashboard:** https://brandmeister.network/

---

**73 and enjoy your ESP32 MMDVM Hotspot!**

*This project is for licensed amateur radio operators only. Not for commercial use.*

**Developed by PD2EMC & PD8JO**
