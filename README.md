# ESP32 RTOS MMDVM

**Work in Progress**

Multi-Mode Digital Voice Modem (MMDVM) implementation for ESP32 with RTOS and web interface.

## Features

- **Multiple Digital Voice Modes:**
  - DMR
  - D-STAR
  - System Fusion (YSF)
  - P25
  - NXDN
  - POCSAG

- **Network Connectivity:**
  - WiFi support
  - Ethernet support

- **System Components:**
  - Web server interface
  - OLED display support
  - SD card logging
  - Sensor monitoring
  - LED status indicators
  - Serial logging

- **Web Interface:**
  - Mode configuration pages
  - System settings
  - Firmware updates
  - Status monitoring
  - MQTT integration

## Project Structure

```
├── esp32_rtos_webserver.ino    # Main Arduino sketch
├── mmdvm_*.cpp                  # MMDVM mode implementations
├── system_*.cpp                 # System component implementations
├── include/                     # Configuration headers
├── mmdvm/                       # MMDVM headers
├── system/                      # System headers
└── web/                         # Web interface assets
```

## Status

This project is currently under active development.
