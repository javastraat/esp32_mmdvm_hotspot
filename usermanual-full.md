# ESP32 MMDVM Hotspot — Full User Manual

This manual provides a complete guide to every feature, page, card, and configuration option in the ESP32 MMDVM Hotspot firmware. Screenshots for each page will be added later.

---

## Table of Contents
1. Introduction
2. Hardware Overview
3. Getting Started
4. Web Interface Overview
5. Main Page (Detailed)
6. System Info Page (Detailed)
7. System Hardware Page (Detailed)
8. System Status Page (Detailed)
9. System Files Page (Detailed)
10. Serial Monitor Page (Detailed)
11. Mode Pages (DMR, POCSAG, DAPNET, P25, D-Star, YSF, NXDN) (Detailed)
12. Service Pages (WiFi, Firmware, Admin, MQTT, WireGuard, SD Card) (Detailed)
13. Card Explanations
14. Configuration & Setup
15. Troubleshooting
16. Glossary
17. Appendix: Source File References
18. Advanced Usage Scenarios & Best Practices

---

## 1. Introduction
The ESP32 MMDVM Hotspot is a digital voice modem supporting multiple amateur radio modes. It features a modular web interface for configuration, monitoring, and management.

## 2. Hardware Overview
- ESP32 microcontroller
- OLED display (optional)
- SD card slot
- Ethernet (W5500)
- Status LED
- User button
- MMDVM modem (UART)

## 3. Getting Started
- Power up the device
- Connect to WiFi or Ethernet
- Access the web interface via browser
- Run the Setup Wizard (enter callsign, DMR ID)

## 4. Web Interface Overview
The web UI is organized into pages, each with dashboard cards displaying status, configuration, or controls. Navigation is via the sidebar or top menu.

---

## 5. Main Page (Detailed)

### Main Page Overview
The Main Page is the central dashboard for your ESP32 MMDVM Hotspot. It provides:
- Real-time monitoring of digital voice and paging activity
- Quick access to recent calls and messages
- Initial setup and configuration controls
- Status of all enabled modes and services

#### How to Use the Main Page
1. **Check your callsign and device hostname** at the top to confirm you are connected to the correct hotspot.
2. **Review the Welcome Card** for a summary of supported features and hardware.
3. **Complete the First-time Setup Card** if prompted, entering your callsign and DMR ID. This is required for operation.
4. **Monitor the On Air Card** for live transmission status, including current mode, last heard station, and timer.
5. **View the Last 15 Calls Card** to see recent DMR activity. Use the duration filter to focus on longer transmissions.
6. **Check the Last Received DAPNET Card** for incoming paging messages (if DAPNET is enabled).
7. **Manage the POCSAG Queue Card** to see messages waiting for transmission.
8. **Review the Last 15 Transmitted POCSAG Card** for a history of sent pager messages.
9. **Preview the OLED Display Card** to see what is shown on the physical OLED screen.
10. **Check the Modes Card** for the status of all enabled digital voice and paging modes.
11. **Review the Services Card** for the status of MQTT, WireGuard, and other system services.

#### Troubleshooting the Main Page
- If your callsign is not displayed, complete the First-time Setup Card.
- If no modes are enabled, go to the Modes Card and enable at least one digital voice or paging mode.
- If you do not see recent calls or messages, check your network connection and modem status.
- For issues with the OLED display, verify hardware connections and settings in the System Hardware Page.
- For service issues (MQTT, WireGuard), check the Services Card for error messages and configuration status.

#### Advanced Tips
- Use the duration filter in the Last 15 Calls Card to focus on longer transmissions, which may indicate important activity.
- Hover over status indicators in the Modes and Services Cards for more details.
- Use the browser’s refresh function to update live data if auto-refresh is disabled.

---

### Welcome Card
**Header:** `<h3>Welcome to ESP32 MMDVM</h3>`
**Purpose:**
- Provides a friendly introduction to the hotspot and its capabilities.
- Lists all supported digital modes:
	- **DMR**: Digital Mobile Radio, for worldwide talkgroups and amateur radio communication.
	- **DAPNET**: Decentralized Amateur Paging Network, for receiving paging messages.
	- **POCSAG**: RF Pager Transmitter, for sending local paging messages.
- Highlights connectivity options:
	- **MQTT**: For publishing activity and integrating with automation platforms.
	- **WireGuard**: For secure remote access via VPN.
	- **WiFi/Ethernet**: For flexible network connections.
- Details hardware features:
	- **OLED Display**: Shows live call info and system status.
	- **SD Card**: Stores DMR user database and logs.

**Example Scenario:**
When first accessing the hotspot, users see this card to understand what the device can do and how it connects to their radio and network.

**Advanced Usage:**
Refer to this card for a summary of all available features before configuring the device.

### First-time Setup Card
**Header:** `<h3 style='color:#dc3545;'>&#9888; First-time Setup</h3>`
**Purpose:**
- Prompts the user to enter their **callsign** and **DMR ID**.
- Includes input fields for both values and a save button.
- Required for initial configuration; the device will not operate until these are set.

**Fields:**
- **Callsign**: Your amateur radio callsign (3-10 characters, uppercase).
- **DMR ID**: Your unique DMR identifier (1-7 digits).

**Example Scenario:**
On first boot, the hotspot displays this card. Enter your callsign and DMR ID, then click Save & Continue. The device reloads and is ready for use.

**Advanced Usage:**
If you need to change your callsign or DMR ID later, revisit this card via the setup menu.

### On Air Card
**Header:** `<div id='onair-card'>` (no explicit h3)
**Purpose:**
- Displays real-time transmission status:
	- **State**: IDLE or active (transmitting/receiving).
	- **Current Mode**: DMR, D-Star, YSF, P25, NXDN, or POCSAG.
	- **Last Heard Station**: Callsign and details of the most recent station.
	- **Timer**: Duration of current transmission.

**Fields:**
- **On Air Badge**: Visual indicator of transmission state.
- **On Air Info**: Details about the current or last transmission.
- **Timer**: Shows how long the transmission has been active.

**Example Scenario:**
During a QSO, the On Air Card updates to show your callsign, mode, and transmission duration. When idle, it displays the last heard station.

**Advanced Usage:**
Use this card to monitor live activity and troubleshoot transmission issues.

### Last 15 Calls Card
**Header:** `<h3>Last 15 Calls</h3>`
**Purpose:**
- Shows a table of the most recent 15 DMR calls received by the hotspot.
- Includes detailed columns:
	- **Time**: Timestamp of each call.
	- **Duration**: Length of transmission.
	- **Callsign**: Station identifier.
	- **Name**: Operator’s name (from DMR database).
	- **City/Country**: Location info (from DMR database).
- Includes a **duration filter** dropdown to focus on longer transmissions.

**Fields:**
- **Call History Table**: Displays all columns above.
- **Duration Filter**: Dropdown to filter calls by minimum duration.

**Example Scenario:**
After a busy day, review this card to see who called, how long they transmitted, and where they are from.

**Advanced Usage:**
Use the filter to identify important or lengthy transmissions for logging or follow-up.

### Last Received DAPNET Card
**Header:** `<h3>Last Received DAPNET</h3>`
**Purpose:**
- Displays the most recent DAPNET paging messages received by the hotspot.
- Columns include:
	- **Time**: When the message was received.
	- **RIC**: Recipient Identification Code.
	- **Message**: Paging content.
- Only visible if DAPNET is enabled in the configuration.

**Fields:**
- **DAPNET History Table**: Shows all received messages with details.

**Example Scenario:**
Monitor this card to see emergency or informational pages sent to your station.

**Advanced Usage:**
Use RIC filtering to track messages for specific pagers.

### POCSAG Queue Card
**Header:** `<h3>POCSAG Queue</h3>`
**Purpose:**
- Shows all POCSAG messages waiting to be transmitted.
- Columns include:
	- **RIC**: Recipient Identification Code.
	- **Message**: Paging content.
- Displays queue status (number of messages pending).

**Fields:**
- **POCSAG Queue Table**: List of all pending messages.
- **Queue Status**: Shows how many messages are in the queue.

**Example Scenario:**
Send a batch of pages and monitor this card to ensure all messages are transmitted.

**Advanced Usage:**
Use queue management to prioritize urgent messages.

### Last 15 Transmitted POCSAG Card
**Header:** `<h3>Last 15 Transmitted POCSAG</h3>`
**Purpose:**
- Displays a history of the last 15 POCSAG messages sent by the hotspot.
- Columns include:
	- **Time**: When the message was sent.
	- **RIC**: Recipient Identification Code.
	- **Message**: Paging content.

**Fields:**
- **POCSAG TX Table**: List of all transmitted messages.
- **TX Count**: Number of messages sent.

**Example Scenario:**
Verify that important pages were sent successfully by reviewing this card.

**Advanced Usage:**
Use this card for auditing and troubleshooting transmission issues.

### OLED Display Card
**Header:** `<h3>OLED Display</h3>`
**Purpose:**
- Provides a virtual preview of the physical OLED screen attached to the hotspot.
- Shows:
	- **Live Call Info**: Current transmission details.
	- **Callsign**: Your station identifier.
	- **System Status**: Device health and activity.
- Includes a **canvas** for real-time rendering of the display.

**Fields:**
- **OLED Canvas**: Graphical preview of the OLED output.

**Example Scenario:**
Remotely monitor what is shown on the OLED without being near the device.

**Advanced Usage:**
Use this card to verify display configuration and troubleshoot rendering issues.

### Modes Card
**Header:** `<h3>Modes</h3>`
**Purpose:**
- Lists all enabled digital voice and paging modes:
	- **DMR, D-Star, YSF, P25, NXDN, POCSAG**
- Shows status indicators for each mode (enabled/disabled, active/inactive).
- Provides configuration links to mode settings pages.

**Fields:**
- **Mode Status List**: Shows all modes and their current state.

**Example Scenario:**
Quickly check which modes are active and access their configuration.

**Advanced Usage:**
Use this card to enable/disable modes and troubleshoot mode-specific issues.

### Services Card
**Header:** `<h3>Services</h3>`
**Purpose:**
- Lists all enabled system services:
	- **MQTT**: For automation and event publishing.
	- **WireGuard**: For secure VPN access.
- Shows status indicators for each service (connected/disconnected, active/inactive).
- Provides configuration links to service settings pages.

**Fields:**
- **Service Status List**: Shows all services and their current state.

**Example Scenario:**
Monitor service health and access configuration for MQTT and VPN.

**Advanced Usage:**
Use this card to troubleshoot service connectivity and automate system events.

---

## 6. System Info Page (Detailed)

### System Info Page Overview
The System Info Page is your diagnostic hub for hardware, memory, RTOS tasks, storage, and software. Use it to:
- Check chip model, revision, and temperature
- Monitor heap and PSRAM usage
- Review RTOS task stack usage and health
- Inspect storage partitions and firmware details
- Track uptime, reset reason, and software versions

#### How to Use the System Info Page
1. **Review System Hardware Card** for chip details and temperature. If temperature is out of range, check cooling and environment.
2. **Monitor Memory Card** for heap and PSRAM usage. Low free heap may indicate memory leaks or excessive load.
3. **Check Task Stack Usage Card** for tasks with low stack. Red or orange indicators mean a task is at risk of overflow.
4. **Expand All Tasks Card** for a full table of RTOS tasks. Use this for advanced debugging and performance analysis.
5. **Inspect Storage Card** for flash size, speed, mode, and sketch details. Ensure free space is sufficient for updates.
6. **Review Software Card** for uptime, reset reason, SDK, and build info. Frequent resets may indicate instability.
7. **Check Modem Information Card** (if present) for modem firmware details. Use this for hardware troubleshooting.

#### Troubleshooting the System Info Page
- If heap or PSRAM is low, reduce running services or check for memory leaks.
- If a task shows red/orange stack usage, optimize code or increase stack allocation.
- If storage is nearly full, delete unused files or update firmware partitions.
- If uptime is low or resets are frequent, check power supply and firmware stability.

#### Advanced Tips
- Use the All Tasks table to identify tasks consuming excessive resources.
- Compare build date and firmware version to ensure you are running the latest software.
- Use the Modem Information Card to verify modem compatibility and firmware.

---

#### System Hardware Card
**Header:** `<h3>System Hardware</h3>`
**Purpose:** Shows ESP32 chip model, revision, unique ID, number of CPU cores, frequency, and temperature. Provides a summary of the hardware platform.
**Fields:**
- **Chip Model:** The specific ESP32 variant (e.g., ESP32-WROOM-32, ESP32-S3). Determines available features and compatibility.
- **Revision:** Hardware revision number, useful for identifying silicon bugs or feature changes.
- **Unique ID:** A hexadecimal identifier unique to each ESP32, used for device tracking and network identification.
- **CPU Cores:** Number of processor cores (typically 2 for ESP32), affects multitasking and performance.
- **Frequency:** Operating clock speed in MHz. Higher values mean faster processing but more power consumption.
- **Temperature:** Real-time reading of the chip temperature. Out-of-range values may indicate overheating or environmental issues.
**Step-by-Step Usage:**
1. Open the System Info Page and locate the System Hardware Card.
2. Verify the chip model and revision match your expected hardware.
3. Check the temperature reading; if above 80°C, ensure proper cooling and ventilation.
4. Use the Unique ID for device registration or remote management.
**Troubleshooting:**
- If temperature is not displayed, check sensor connection and firmware version.
- If chip model or revision is incorrect, verify hardware and update firmware.
**Advanced Tips:**
- Use the Unique ID for secure network authentication.
- Compare revision numbers when troubleshooting hardware-specific issues.
**Example Scenario:**
You notice frequent resets; check the temperature field to see if overheating is the cause.

#### Memory Card
**Header:** `<h3>Memory</h3>`
**Purpose:** Displays heap size, free heap, minimum free heap, maximum allocatable heap, and PSRAM details (size, free, max alloc). Useful for monitoring memory usage and diagnosing low-memory conditions.
**Fields:**
- **Heap Size:** Total available RAM for dynamic allocation.
- **Free Heap:** Current unused RAM. Low values may cause instability.
- **Minimum Free Heap:** Lowest recorded free heap since boot. Useful for spotting memory leaks.
- **Maximum Allocatable Heap:** Largest block of memory that can be allocated at once.
- **PSRAM Size:** Total pseudo-static RAM, if available.
- **PSRAM Free:** Unused PSRAM.
- **PSRAM Max Alloc:** Largest allocatable PSRAM block.
**Step-by-Step Usage:**
1. Monitor Free Heap and PSRAM Free during normal operation.
2. If Free Heap drops below 10kB, consider disabling unused services.
3. Use Minimum Free Heap to identify memory leaks over time.
**Troubleshooting:**
- If Free Heap is consistently low, check for runaway tasks or excessive logging.
- If PSRAM is not detected, verify hardware and firmware support.
**Advanced Tips:**
- Use Maximum Allocatable Heap to size large buffers for audio or data processing.
- Track Minimum Free Heap after firmware updates to spot regressions.
**Example Scenario:**
After enabling DMR and DAPNET, Free Heap drops sharply; disable one mode to restore stability.

#### Task Stack Usage Card
**Header:** `<h3>Task Stack Usage</h3>`
**Purpose:** Lists all RTOS tasks with their stack usage, color-coded for low memory. Helps identify tasks at risk of stack overflow.
**Fields:**
- **Task Name:** Identifier for each FreeRTOS task (e.g., Main, WebServer, Modem).
- **Stack Usage:** Amount of stack memory used, shown as a percentage and color-coded (green, orange, red).
- **Stack Free:** Remaining stack space for each task.
- **Priority:** Task scheduling priority.
- **Core:** Which CPU core the task runs on.
**Step-by-Step Usage:**
1. Review the color codes; red means immediate action is needed.
2. Expand the card for full details on all tasks.
3. Increase stack allocation for tasks in orange/red if possible.
**Troubleshooting:**
- If a task overflows, the device may crash or reset. Check stack usage after adding new features.
- If stack usage is high, optimize code or split tasks.
**Advanced Tips:**
- Use this card to tune multitasking performance and avoid resource contention.
**Example Scenario:**
After enabling WireGuard, the WebServer task shows orange; increase its stack size in config.h.

#### All Tasks Card
**Header:** `<h3>All Tasks</h3>`
**Purpose:** Table of all RTOS tasks, showing name, state, priority, core, and stack free. Expandable for full details. Useful for advanced debugging and system monitoring.
**Fields:**
- **Task Name:** Name of each running task.
- **State:** Current state (Running, Blocked, Suspended).
- **Priority:** Scheduling priority.
- **Core:** CPU core assignment.
- **Stack Free:** Remaining stack memory.
**Step-by-Step Usage:**
1. Expand the card to see all running tasks.
2. Identify tasks with low stack free and high priority.
3. Use this info for advanced debugging and performance tuning.
**Troubleshooting:**
- If a task is stuck in Blocked/Suspended, check for resource deadlocks.
- If stack free is low, increase allocation or optimize code.
**Advanced Tips:**
- Use task info to balance load across CPU cores.
**Example Scenario:**
During heavy network traffic, the MQTT task shows low stack free; optimize message handling.

#### Storage Card
**Header:** `<h3>Storage</h3>`
**Purpose:** Shows flash size, speed, mode, sketch size, free space, MD5, and running partition. Useful for firmware management and storage diagnostics.
**Fields:**
- **Flash Size:** Total onboard storage capacity.
- **Flash Speed:** Operating speed in MHz.
- **Flash Mode:** SPI mode (DIO, QIO, etc.).
- **Sketch Size:** Size of the current firmware image.
- **Free Space:** Remaining space for updates and files.
- **MD5:** Hash of the running firmware for integrity checking.
- **Running Partition:** Active firmware partition.
**Step-by-Step Usage:**
1. Check Free Space before performing OTA updates.
2. Verify MD5 after firmware upload to ensure integrity.
3. Use Flash Mode and Speed for advanced hardware tuning.
**Troubleshooting:**
- If Free Space is low, delete unused files or expand storage.
- If MD5 does not match, re-upload firmware.
**Advanced Tips:**
- Use multiple partitions for safe firmware updates.
**Example Scenario:**
OTA update fails due to low Free Space; clear logs and retry.

#### Software Card
**Header:** `<h3>Software</h3>`
**Purpose:** Displays uptime, reset reason, SDK version, Arduino core version, build date/time, and (optionally) firmware version. Useful for tracking system stability and software environment.
**Fields:**
- **Uptime:** Time since last reset.
- **Reset Reason:** Cause of last reset (Power-on, Watchdog, Software, etc.).
- **SDK Version:** ESP-IDF or Arduino core version.
- **Build Date/Time:** When the firmware was compiled.
- **Firmware Version:** Optional, for tracking releases.
**Step-by-Step Usage:**
1. Monitor Uptime for stability; frequent resets may indicate issues.
2. Check Reset Reason after unexpected reboots.
3. Compare SDK and build info to ensure you are running the latest firmware.
**Troubleshooting:**
- If resets are frequent, check power supply and firmware logs.
- If SDK version is outdated, update firmware.
**Advanced Tips:**
- Use build info for bug reporting and diagnostics.
**Example Scenario:**
Device resets every hour; Reset Reason shows Watchdog, indicating a stuck task.

#### Modem Information Card (optional)
**Header:** `<h3>Modem Information</h3>`
**Purpose:** Shows modem hardware, firmware version, build date, crystal, transceiver, author, and Git ID. Only present if modem firmware version tracking is enabled.
**Fields:**
- **Modem Hardware:** Model and revision of the modem board.
- **Firmware Version:** Current modem firmware.
- **Build Date:** When modem firmware was compiled.
- **Crystal:** Reference oscillator frequency.
- **Transceiver:** Type of RF chip used.
- **Author:** Firmware author or maintainer.
- **Git ID:** Source code commit identifier.
**Step-by-Step Usage:**
1. Check Modem Hardware and Firmware Version for compatibility.
2. Use Build Date and Git ID for troubleshooting and support.
**Troubleshooting:**
- If modem is not detected, check hardware connections and firmware compatibility.
- If firmware version is outdated, update modem firmware.
**Advanced Tips:**
- Use Git ID for precise bug tracking and support requests.
**Example Scenario:**
After a firmware update, modem fails to initialize; check Modem Information Card for mismatched versions.

---

## 7. System Hardware Page (Detailed)

### System Hardware Page Overview
The System Hardware Page is where you configure all physical connections and peripherals. Use it to:
- Set GPIO pins for LED and button
- Enable and configure OLED display
- Enable and configure SD card SPI interface
- Save and reboot hardware settings

#### How to Use the System Hardware Page
1. **Set LED & Button Settings Card** to match your hardware wiring. Use Save/Reset buttons to apply or revert changes.
2. **Configure OLED Settings Card** for display type, I2C pins, address, and resolution. Enable OLED for live status.
3. **Configure SD Card Settings Card** for SPI pins and enable SD card for database and file storage.
4. **Click Save All & Reboot** after making changes to apply settings and restart the device.

#### Troubleshooting the System Hardware Page
- If LED or button does not work, check pin assignments and wiring.
- If OLED does not display, verify I2C pins, address, and enable setting.
- If SD card is not detected, check SPI pins and card format.
- Use Reset to Default to restore factory settings if needed.

#### Advanced Tips
- Use advanced pin configuration for custom hardware setups.
- Match OLED resolution to your display for best results.
- Use SD card for large databases and log storage.

---

#### LED & Button Settings Card
**Header:** `<h3>LED & Button Settings</h3>`
**Purpose:** Configure GPIO pins for the status LED and OLED toggle button. Includes input fields for pin numbers and save/reset buttons.
**Fields:**
- **LED Pin:** GPIO number for the status LED. Indicates device state (on, off, blinking for activity/errors).
- **Button Pin:** GPIO number for the user button. Used to toggle OLED display or trigger user actions.
- **Save Button:** Saves current pin configuration to non-volatile storage.
- **Reset Button:** Restores default pin settings.
**Step-by-Step Usage:**
1. Enter the correct GPIO numbers for your hardware wiring.
2. Click Save to apply changes; the device will update pin assignments immediately.
3. Use Reset to revert to factory defaults if needed.
**Troubleshooting:**
- If LED does not light, check wiring and pin assignment.
- If button is unresponsive, verify pin number and physical connection.
**Advanced Tips:**
- Use alternate GPIOs for custom hardware setups.
- Assign button to trigger additional actions via firmware customization.
**Example Scenario:**
You install a new LED on GPIO 2; update the pin in this card and click Save.

#### OLED Settings Card
**Header:** `<h3>OLED Settings</h3>`
**Purpose:**
- Enable/disable OLED display, configure I2C pins, address, width, and height. Includes advanced pin configuration and save/reset controls.
**Fields:**
- **Enable OLED:** Checkbox to turn OLED display on or off.
- **I2C SDA Pin:** GPIO for I2C data line.
- **I2C SCL Pin:** GPIO for I2C clock line.
- **I2C Address:** Hex address of the OLED module (usually 0x3C or 0x78).
- **Width/Height:** Pixel dimensions of the display (e.g., 128x64).
- **Save Button:** Saves OLED configuration.
- **Reset Button:** Restores default OLED settings.
**Step-by-Step Usage:**
1. Enable OLED and enter correct I2C pins and address for your display.
2. Set width and height to match your OLED module.
3. Click Save to apply changes; the display will update immediately.
4. Use Reset to revert to factory defaults.
**Troubleshooting:**
- If OLED does not display, check wiring, I2C pins, and address.
- If display is garbled, verify width/height settings.
**Advanced Tips:**
- Use custom I2C addresses for multiple displays.
- Adjust resolution for different OLED models.
**Example Scenario:**
You upgrade to a 128x32 OLED; update width/height and save settings.

#### SD Card Settings Card
**Header:** `<h3>SD Card Settings</h3>`
**Purpose:** Enable/disable SD card, configure SPI pins (MISO, MOSI, SCLK, CS). Includes advanced pin configuration and save/reset controls.
**Fields:**
- **Enable SD Card:** Checkbox to turn SD card support on or off.
- **SPI MISO Pin:** GPIO for SPI data in.
- **SPI MOSI Pin:** GPIO for SPI data out.
- **SPI SCLK Pin:** GPIO for SPI clock.
- **SPI CS Pin:** GPIO for chip select.
- **Save Button:** Saves SD card configuration.
- **Reset Button:** Restores default SD card settings.
**Step-by-Step Usage:**
1. Enable SD card and enter correct SPI pin numbers for your hardware.
2. Click Save to apply changes; the device will attempt to mount the SD card.
3. Use Reset to revert to factory defaults.
**Troubleshooting:**
- If SD card is not detected, check hardware and format (FAT32 recommended).
- If file operations fail, verify card integrity and free space.
**Advanced Tips:**
- Use high-speed SD cards for large databases and logs.
- Assign alternate SPI pins for custom hardware.
**Example Scenario:**
You add an SD card module; configure pins and enable support in this card.

#### Save All & Reboot
**Button:** `Save All & Reboot`
**Purpose:** Applies all hardware settings and reboots the device for changes to take effect.
**Step-by-Step Usage:**
1. After configuring LED, OLED, and SD card settings, click Save All & Reboot.
2. Device will restart and apply all new hardware configurations.
**Troubleshooting:**
- If device does not reboot, check power supply and firmware stability.
- If settings are not applied, verify save operations and try again.
**Advanced Tips:**
- Use Save All & Reboot after multiple changes to ensure all settings are synchronized.
**Example Scenario:**
You change both OLED and SD card settings; use this button to apply all changes at once.

---

## 8. System Status Page (Detailed)

### System Status Page Overview
The System Status Page is your live monitor for station identity, network, modem, and VPN health. Use it to:
- Confirm callsign, DMR ID, and enabled modes
- Check WiFi and Ethernet connection status
- Monitor modem health and firmware
- Review WireGuard VPN status

#### How to Use the System Status Page
1. **Review Station Information Card** for callsign, DMR ID, SSID, and enabled modes. Ensure correct configuration.
2. **Check WiFi Status Card** for connection details, signal strength, and AP mode info.
3. **Check Ethernet Status Card** for wired connection status and details (if enabled).
4. **Monitor MMDVM Hardware Status Card** for modem readiness, frequencies, and firmware info.
5. **Review WireGuard Status Card** for VPN connection, endpoint, and allowed IPs.

#### Troubleshooting the System Status Page
- If station info is incorrect, update callsign and DMR ID in setup.
- If WiFi/Ethernet is disconnected, check network settings and cables.
- If modem is not ready, check hardware connections and firmware.
- If VPN is not connected, verify WireGuard configuration and endpoint.

#### Advanced Tips
- Use MAC address and link speed info for advanced network diagnostics.
- Monitor modem build date and Git ID for firmware tracking.
- Use allowed IPs and DNS info for VPN routing.

---

#### Station Information Card
**Header:** `<h3>Station Information</h3>`
**Purpose:** Shows callsign, DMR ID, SSID, and enabled modes. Indicates configuration status and provides a summary of station identity.
**Fields:**
- **Callsign:** Your amateur radio callsign, confirming device identity.
- **DMR ID:** Unique identifier for DMR networks.
- **SSID:** WiFi network name the device is connected to.
- **Enabled Modes:** List of all active digital voice and paging modes.
- **Status Indicator:** Visual cue (color or icon) for configuration completeness.
**Step-by-Step Usage:**
1. Confirm your callsign and DMR ID are correct; update via setup if needed.
2. Check SSID to verify network connection.
3. Review enabled modes to ensure desired operation.
**Troubleshooting:**
- If callsign or DMR ID is missing, complete setup via the First-time Setup Card.
- If SSID is incorrect, reconnect to the correct WiFi network.
**Advanced Tips:**
- Use enabled modes list to quickly verify system configuration after firmware updates.
**Example Scenario:**
You change your DMR ID; verify the update here before transmitting.

#### WiFi Status Card
**Header:** `<h3>WiFi Status</h3>`
**Purpose:** Displays WiFi connection status, SSID, IP address, gateway, subnet, DNS, signal strength, channel, MAC address, and AP mode info. Useful for network diagnostics.
**Fields:**
- **SSID:** Name of the connected WiFi network.
- **IP Address:** Device’s assigned IP on the network.
- **Gateway:** Router IP address.
- **Subnet:** Network mask.
- **DNS:** Domain Name Server address.
- **Signal Strength:** RSSI value, shown as bars or dBm.
- **Channel:** WiFi channel in use.
- **MAC Address:** Device’s hardware network address.
- **AP Mode Info:** Indicates if device is running as an access point.
- **Scan Button:** Initiates scan for available networks.
- **Connect Button:** Connects to selected network.
**Step-by-Step Usage:**
1. Review SSID and signal strength for connection quality.
2. Use Scan to find available networks; select and Connect as needed.
3. Check IP, gateway, and DNS for troubleshooting connectivity.
**Troubleshooting:**
- If signal is weak, move device closer to router.
- If IP is missing, check DHCP settings.
**Advanced Tips:**
- Use AP mode for direct device access in the field.
**Example Scenario:**
You move the hotspot to a new location; use Scan and Connect to join a new WiFi network.

#### Ethernet Status Card
**Header:** `<h3>Ethernet Status</h3>`
**Purpose:** Shows Ethernet connection status, IP, gateway, subnet, DNS, MAC address, link speed, and duplex. Only present if Ethernet is enabled.
**Fields:**
- **Connection Status:** Indicates if Ethernet is active.
- **IP Address:** Device’s wired network IP.
- **Gateway:** Router IP address.
- **Subnet:** Network mask.
- **DNS:** Domain Name Server address.
- **MAC Address:** Hardware address for Ethernet.
- **Link Speed:** Connection speed (e.g., 100Mbps).
- **Duplex:** Full or half duplex mode.
**Step-by-Step Usage:**
1. Confirm connection status is active.
2. Review IP, gateway, and DNS for network diagnostics.
3. Check link speed and duplex for optimal performance.
**Troubleshooting:**
- If connection is down, check cable and router.
- If link speed is low, verify cable and switch compatibility.
**Advanced Tips:**
- Use full duplex for best performance in busy networks.
**Example Scenario:**
You connect the hotspot to a new switch; verify link speed and duplex here.

#### MMDVM Hardware Status Card
**Header:** `<h3>MMDVM Hardware Status</h3>`
**Purpose:** Indicates modem readiness, RX/TX frequency, color code, power level, modem hardware, firmware version, build date, crystal, chip, author, and Git ID. Useful for hardware diagnostics.
**Fields:**
- **Modem Ready:** Shows if modem is initialized and ready.
- **RX/TX Frequency:** Current receive/transmit frequencies.
- **Color Code:** DMR color code in use.
- **Power Level:** RF output power setting.
- **Modem Hardware:** Model and revision.
- **Firmware Version:** Current modem firmware.
- **Build Date:** Firmware compilation date.
- **Crystal:** Reference oscillator frequency.
- **Chip:** RF chip type.
- **Author:** Firmware author.
- **Git ID:** Source code commit identifier.
- **Reboot Modem Button:** Restarts modem hardware.
**Step-by-Step Usage:**
1. Confirm modem is ready before transmitting.
2. Review RX/TX frequencies and color code for correct operation.
3. Use Reboot Modem if hardware is unresponsive.
**Troubleshooting:**
- If modem is not ready, check hardware connections and firmware.
- If frequencies are incorrect, update settings in mode configuration.
**Advanced Tips:**
- Use Git ID for precise bug tracking and support.
**Example Scenario:**
After a firmware update, modem fails to initialize; use Reboot Modem and check status fields.

#### WireGuard Status Card
**Header:** `<h3>WireGuard Status</h3>`
**Purpose:** Shows VPN connection status, local IP, endpoint, port, DNS, and allowed IPs. Useful for secure remote access and VPN diagnostics.
**Fields:**
- **VPN Connection Status:** Indicates if tunnel is active.
- **Local IP:** Device’s VPN-assigned IP.
- **Endpoint:** Remote VPN server address.
- **Port:** WireGuard UDP port.
- **DNS:** VPN DNS server.
- **Allowed IPs:** List of IPs routed through VPN.
- **Connect/Disconnect Button:** Toggles VPN connection.
**Step-by-Step Usage:**
1. Confirm VPN connection status is active for remote access.
2. Review endpoint and allowed IPs for correct routing.
3. Use Connect/Disconnect to manage VPN session.
**Troubleshooting:**
- If VPN does not connect, verify endpoint and credentials.
- If routing is incorrect, update allowed IPs.
**Advanced Tips:**
- Use VPN for secure remote management and telemetry.
**Example Scenario:**
You need to access the hotspot remotely; use Connect to establish VPN and verify local IP.

---

## 9. System Files Page (Detailed)

### System Files Page Overview
The System Files Page is your file manager for internal flash and SD card storage. Use it to:
- Browse, upload, download, and delete files
- Install bootlogo packs
- Manage databases and logs

#### How to Use the System Files Page
1. **Use LittleFS File Browser Card** to manage files in internal flash. Upload new files or download backups.
2. **Use SD Card Browser Card** to manage files on the SD card. Store large databases and logs here.
3. **Use Bootlogos Installer Card** to download and install new bootlogo packs for the OLED display.

#### Troubleshooting the System Files Page
- If files do not upload/download, check browser compatibility and network connection.
- If SD card is not detected, check hardware and format.
- If bootlogo install fails, verify file integrity and storage space.

#### Advanced Tips
- Use SD card for large databases to improve performance.
- Regularly backup configuration and log files.
- Use file browser to update firmware manually if needed.

---

#### LittleFS File Browser Card
**Header:** `<h3>LittleFS File Browser</h3>`
**Purpose:** Browse, upload, download, and delete files stored in internal flash memory.
**Fields:**
- **File List:** Table of all files in internal flash, showing name, size, and date modified.
- **Upload Button:** Opens file picker to upload new files to flash.
- **Download Button:** Downloads selected file to your computer.
- **Delete Button:** Removes selected file from flash storage.
- **Refresh Button:** Reloads file list to show latest changes.
**Step-by-Step Usage:**
1. Use File List to browse available files.
2. Click Upload to add new files (e.g., config, database).
3. Select a file and click Download to save a backup.
4. Select a file and click Delete to remove it.
5. Click Refresh after file operations to update the list.
**Troubleshooting:**
- If upload fails, check file size and format.
- If download does not start, check browser compatibility.
- If delete fails, verify file permissions and free space.
**Advanced Tips:**
- Use file browser to manually update firmware or configuration files.
- Regularly backup important files before making changes.
**Example Scenario:**
You want to update the DMR database; upload the new file and delete the old one.

#### SD Card Browser Card
**Header:** `<h3>SD Card Browser</h3>`
**Purpose:** Browse, upload, download, and delete files stored on the SD card.
**Fields:**
- **File List:** Table of all files on SD card, showing name, size, and date modified.
- **Upload Button:** Opens file picker to upload new files to SD card.
- **Download Button:** Downloads selected file to your computer.
- **Delete Button:** Removes selected file from SD card.
- **Refresh Button:** Reloads file list to show latest changes.
**Step-by-Step Usage:**
1. Use File List to browse SD card contents.
2. Click Upload to add new files (e.g., logs, databases).
3. Select a file and click Download to save a backup.
4. Select a file and click Delete to remove it.
5. Click Refresh after file operations to update the list.
**Troubleshooting:**
- If SD card is not detected, check hardware and format (FAT32 recommended).
- If upload/download fails, check file size and browser compatibility.
- If delete fails, verify file permissions and free space.
**Advanced Tips:**
- Use SD card for large databases and log files to save internal flash space.
- Regularly backup SD card files to your computer.
**Example Scenario:**
You want to archive log files; download them from SD card and delete old entries.

#### Bootlogos Installer Card
**Header:** `<h3>Bootlogos Installer</h3>`
**Purpose:** Download and install bootlogo packs for the device’s display.
**Fields:**
- **Bootlogo List:** Table of available bootlogo packs, showing name and preview.
- **Download Button:** Downloads selected bootlogo pack from server.
- **Install Button:** Installs selected bootlogo pack to device.
- **Delete Button:** Removes bootlogo pack from device storage.
- **Refresh Button:** Reloads bootlogo list to show latest changes.
**Step-by-Step Usage:**
1. Browse Bootlogo List to preview available packs.
2. Click Download to fetch new bootlogo packs.
3. Select a pack and click Install to apply it to the device.
4. Select a pack and click Delete to remove it.
5. Click Refresh after operations to update the list.
**Troubleshooting:**
- If download fails, check network connection and server status.
- If install fails, verify free space and file integrity.
- If delete fails, check file permissions.
**Advanced Tips:**
- Use custom bootlogos for club branding or personal identification.
- Regularly update bootlogos for new features or events.
**Example Scenario:**
You want to display your club logo on startup; download and install the custom bootlogo pack.

---

## 10. Serial Monitor Page (Detailed)

### Serial Monitor Page Overview
The Serial Monitor Page is your live log viewer and UART status monitor. Use it to:
- View real-time serial logs
- Copy and clear log messages
- Monitor UART status and modem health

#### How to Use the Serial Monitor Page
1. **Use Log Viewer Card** to monitor live logs. Enable auto-scroll for continuous updates.
2. **Use Status Bar Card** to check UART status, baud rate, RX/TX pins, and modem health.
3. **Copy or clear logs** as needed for troubleshooting or record-keeping.

#### Troubleshooting the Serial Monitor Page
- If logs do not appear, check UART connections and baud rate settings.
- If modem health is poor, check hardware and firmware.
- If log copy/clear fails, check browser compatibility.

#### Advanced Tips
- Use log viewer to diagnose firmware and hardware issues.
- Monitor RX/TX pin status for UART troubleshooting.
- Save logs before firmware updates for diagnostics.

---

#### Log Viewer Card
**Header:** `<h3>Log Viewer</h3>`
**Purpose:** Shows serial log messages, with auto-scroll, copy, and clear controls.
**Fields:**
- **Log Output:** Scrollable area displaying real-time serial messages from the device.
- **Auto-Scroll Checkbox:** Toggles automatic scrolling to the latest log entry.
- **Copy Button:** Copies all log messages to clipboard for analysis or sharing.
- **Clear Button:** Deletes all current log messages from the viewer (does not affect device logs).
- **Refresh Button:** Reloads log output from the device.
**Step-by-Step Usage:**
1. Monitor Log Output for real-time system and modem events.
2. Enable Auto-Scroll for continuous updates during troubleshooting.
3. Use Copy to save logs before firmware updates or for bug reports.
4. Use Clear to reset the viewer for new sessions.
5. Click Refresh to reload logs after device events.
**Troubleshooting:**
- If logs do not appear, check UART connections and baud rate settings.
- If Copy or Clear fails, check browser compatibility.
**Advanced Tips:**
- Use logs to diagnose firmware, hardware, and network issues.
- Save logs before major configuration changes for diagnostics.
**Example Scenario:**
You encounter a modem error; copy logs and send to support for analysis.

#### Status Bar Card
**Header:** `<h3>Status Bar</h3>`
**Purpose:** Displays UART status, baud rate, RX/TX pins, and modem health.
**Fields:**
- **UART Status:** Indicates if serial connection is active.
- **Baud Rate:** Current communication speed (e.g., 115200).
- **RX Pin:** GPIO number for UART receive.
- **TX Pin:** GPIO number for UART transmit.
- **Modem Health Indicator:** Visual cue (color or icon) for modem status.
- **Reconnect Button:** Attempts to re-establish UART connection if lost.
**Step-by-Step Usage:**
1. Confirm UART Status is active for log monitoring.
2. Check Baud Rate matches your hardware configuration.
3. Use Reconnect if connection drops or logs stop updating.
**Troubleshooting:**
- If UART is inactive, check cable, pin assignments, and device power.
- If modem health is poor, review logs and check hardware.
**Advanced Tips:**
- Use custom baud rates for advanced hardware setups.
- Monitor RX/TX pin status for troubleshooting serial issues.
**Example Scenario:**
You change UART wiring; update RX/TX pins and use Reconnect to restore logging.

---

## 11. Mode Pages (DMR, POCSAG, DAPNET, P25, D-Star, YSF, NXDN) (Detailed)

### Mode Pages Overview
Each Mode Page is dedicated to a specific digital voice or paging mode. Use them to:
- Enable/disable modes
- Configure IDs, frequencies, talkgroups, and network settings
- View live mode status and diagnostics

#### How to Use the Mode Pages
1. **Use Mode Status Card** to enable/disable the mode and review configuration options.
2. **Use Settings Cards** to set IDs, frequencies, talkgroups, and network/server settings for each mode.
3. **Monitor live status** for activity and diagnostics.

#### Troubleshooting the Mode Pages
- If a mode does not activate, check configuration and hardware compatibility.
- If settings do not save, check browser and firmware version.
- If live status is missing, check network and modem health.

#### Advanced Tips
- Use talkgroup and network settings for advanced routing and connectivity.
- Monitor mode status for firmware and hardware diagnostics.
- Use placeholders for future mode expansion.

---

#### Mode Status Card
**Header:** `<h3>Mode Status</h3>`
**Purpose:** Shows enabled/disabled state and configuration options for the selected mode.
**Fields:**
- **Enable/Disable Toggle:** Switch to activate or deactivate the mode.
- **Status Indicator:** Visual cue (color or icon) for mode activity (active, idle, error).
- **Current State:** Text showing if the mode is running, idle, or in error.
- **Mode Info:** Brief summary of mode type and protocol.
- **Save Button:** Applies changes to mode status.
**Step-by-Step Usage:**
1. Use Enable/Disable Toggle to activate the desired mode.
2. Review Status Indicator and Current State for live feedback.
3. Click Save after making changes to apply them.
**Troubleshooting:**
- If mode does not activate, check hardware compatibility and configuration.
- If error is shown, review logs and mode settings.
**Advanced Tips:**
- Use mode toggles to quickly switch between digital voice and paging protocols.
**Example Scenario:**
You want to test DMR; enable the mode and verify status before transmitting.

#### Settings Cards
**Header:** `<h3>Settings</h3>` (varies by mode)
**Purpose:** Configure IDs, frequencies, talkgroups, network/server settings, and other mode-specific parameters.
**Fields (DMR Example):**
- **DMR ID:** Unique identifier for DMR network.
- **Frequency:** RX/TX frequency in MHz.
- **Color Code:** DMR color code.
- **Talkgroup:** Group ID for communication.
- **Network Server:** Address of DMR server.
- **Save Button:** Applies configuration changes.
- **Reset Button:** Restores default settings.
**Fields (POCSAG Example):**
- **RIC:** Recipient Identification Code.
- **Message:** Paging content.
- **Frequency:** Transmission frequency.
- **Save Button:** Applies configuration changes.
- **Reset Button:** Restores default settings.
**Fields (DAPNET Example):**
- **DAPNET API Key:** Authentication token for DAPNET.
- **Server Address:** DAPNET server URL.
- **RIC:** Recipient Identification Code.
- **Save Button:** Applies configuration changes.
- **Reset Button:** Restores default settings.
**Step-by-Step Usage:**
1. Enter required parameters (ID, frequency, talkgroup, etc.) for your mode.
2. Click Save to apply changes; mode will restart if needed.
3. Use Reset to revert to factory defaults.
**Troubleshooting:**
- If settings do not save, check browser compatibility and firmware version.
- If mode does not operate, verify all required fields are filled and correct.
**Advanced Tips:**
- Use advanced settings for custom routing, encryption, or network integration.
- Save configuration before firmware updates to avoid data loss.
**Example Scenario:**
You change DMR frequency; update the field and click Save to apply.

---

## 12. Service Pages (WiFi, Firmware, Admin, MQTT, WireGuard, SD Card) (Detailed)

### Service Pages Overview
Service Pages are your control center for all major system services. Use them to:
- Configure WiFi and network settings
- Update firmware and manage OTA
- Perform admin actions (reset, reboot, clear logs)
- Configure MQTT and WireGuard VPN
- Manage SD card status and files

#### How to Use the Service Pages
1. **Use WiFi Status Card** to scan for networks, select slots, and configure AP settings.
2. **Use Firmware Status Card** to check ESP32 and modem firmware versions, OTA partition, and update progress.
3. **Use Admin Actions Card** for factory reset, reboot, restart services, and clear logs.
4. **Use MQTT Status Card** to configure broker, publish/subscribe, and token settings.
5. **Use WireGuard Status Card** to set up VPN tunnel, endpoint, allowed IPs, and DNS.
6. **Use SD Card Status and File Browser Cards** to manage SD card files and status.

#### Troubleshooting the Service Pages
- If WiFi does not connect, check credentials and signal strength.
- If firmware update fails, check partition space and file integrity.
- If admin actions do not work, check browser and firmware version.
- If MQTT or WireGuard is not working, verify configuration and network access.
- If SD card is not detected, check hardware and format.

#### Advanced Tips
- Use multiple WiFi slots for quick network switching.
- Use OTA updates for remote firmware management.
- Use MQTT and WireGuard for advanced automation and secure remote access.
- Regularly backup SD card files and configuration.

---

#### WiFi Status Card
**Header:** `<h3>WiFi Status</h3>`
**Purpose:** List available networks, show current slot, and configure AP settings.
**Fields:**
- **Network List:** Table of available WiFi networks with SSID, signal strength, and security type.
- **Current Slot:** Indicates which WiFi slot is active (for multi-slot configuration).
- **SSID Field:** Input for network name.
- **Password Field:** Input for WiFi password.
- **AP Mode Toggle:** Switch to enable/disable access point mode.
- **Scan Button:** Scans for available networks.
- **Connect Button:** Connects to selected network.
- **Save Button:** Saves WiFi configuration.
- **Reset Button:** Restores default WiFi settings.
**Step-by-Step Usage:**
1. Click Scan to list available networks.
2. Select a network and enter password if required.
3. Use AP Mode Toggle to enable hotspot mode if needed.
4. Click Connect to join the network.
5. Click Save to store configuration.
6. Use Reset to revert to factory defaults if needed.
**Troubleshooting:**
- If connection fails, check credentials and signal strength.
- If AP mode does not work, verify device supports hotspot mode.
**Advanced Tips:**
- Use multiple slots for quick switching between networks.
- Enable AP mode for direct access in the field.
**Example Scenario:**
You travel to a new site; scan for networks, select one, and connect.

#### Firmware Status Card
**Header:** `<h3>Firmware Status</h3>`
**Purpose:** Display ESP32 and modem firmware versions, OTA partition, and update progress.
**Fields:**
- **ESP32 Firmware Version:** Current running firmware version.
- **Modem Firmware Version:** Current modem firmware version.
- **OTA Partition:** Indicates which partition is active for updates.
- **Update Progress Bar:** Shows status of firmware update.
- **Update Button:** Initiates firmware update process.
- **Rollback Button:** Reverts to previous firmware version.
- **Save Button:** Saves firmware configuration.
**Step-by-Step Usage:**
1. Review current firmware versions for ESP32 and modem.
2. Click Update to start firmware update; monitor progress bar.
3. Use Rollback if update fails or issues arise.
4. Click Save to store configuration.
**Troubleshooting:**
- If update fails, check OTA partition space and file integrity.
- If rollback does not work, verify previous firmware is available.
**Advanced Tips:**
- Use OTA updates for remote management.
- Rollback after failed updates to restore stability.
**Example Scenario:**
You receive a new firmware release; update via this card and monitor progress.

#### Admin Actions Card
**Header:** `<h3>Admin Actions</h3>`
**Purpose:** Factory reset, reboot, restart services, and clear logs.
**Fields:**
- **Factory Reset Button:** Restores all settings to factory defaults.
- **Reboot Button:** Restarts the device.
- **Restart Services Button:** Restarts all system services without rebooting.
- **Clear Logs Button:** Deletes all system and modem logs.
**Step-by-Step Usage:**
1. Use Factory Reset to restore device to original state.
2. Click Reboot to restart device after major changes.
3. Use Restart Services to refresh system without full reboot.
4. Click Clear Logs to remove old log entries.
**Troubleshooting:**
- If reset does not work, check firmware version and device health.
- If logs do not clear, check file permissions.
**Advanced Tips:**
- Use restart services for quick recovery after configuration changes.
- Clear logs before troubleshooting new issues.
**Example Scenario:**
You change network settings; reboot device to apply changes.

#### MQTT Status Card
**Header:** `<h3>MQTT Status</h3>`
**Purpose:** Show broker connection, publish/subscribe status, and token.
**Fields:**
- **Broker Address Field:** Input for MQTT broker URL or IP.
- **Port Field:** Input for broker port.
- **Token Field:** Input for authentication token.
- **Publish Status Indicator:** Shows if device is publishing events.
- **Subscribe Status Indicator:** Shows if device is receiving events.
- **Connect Button:** Initiates connection to broker.
- **Disconnect Button:** Ends broker connection.
- **Save Button:** Saves MQTT configuration.
- **Reset Button:** Restores default MQTT settings.
**Step-by-Step Usage:**
1. Enter broker address, port, and token as required.
2. Click Connect to establish MQTT session.
3. Monitor publish/subscribe indicators for activity.
4. Click Save to store configuration.
5. Use Reset to revert to defaults.
**Troubleshooting:**
- If connection fails, check broker address and network access.
- If publish/subscribe does not work, verify token and topic settings.
**Advanced Tips:**
- Use MQTT for automation and remote monitoring.
- Secure broker with authentication token.
**Example Scenario:**
You set up home automation; connect device to MQTT broker and monitor events.

#### WireGuard Status Card
**Header:** `<h3>WireGuard Status</h3>`
**Purpose:** Show tunnel state, endpoint, allowed IPs, and DNS.
**Fields:**
- **Tunnel State Indicator:** Shows if VPN tunnel is active.
- **Endpoint Field:** Input for remote VPN server address.
- **Port Field:** Input for WireGuard UDP port.
- **Allowed IPs Field:** Input for IPs routed through VPN.
- **DNS Field:** Input for VPN DNS server.
- **Connect Button:** Establishes VPN tunnel.
- **Disconnect Button:** Ends VPN session.
- **Save Button:** Saves WireGuard configuration.
- **Reset Button:** Restores default VPN settings.
**Step-by-Step Usage:**
1. Enter endpoint, port, allowed IPs, and DNS as required.
2. Click Connect to start VPN session.
3. Monitor tunnel state indicator for activity.
4. Click Save to store configuration.
5. Use Reset to revert to defaults.
**Troubleshooting:**
- If tunnel does not connect, check endpoint and credentials.
- If routing is incorrect, update allowed IPs and DNS.
**Advanced Tips:**
- Use VPN for secure remote access and telemetry.
- Route only necessary IPs for security.
**Example Scenario:**
You need secure remote access; configure WireGuard and connect.

#### SD Card Status Card
**Header:** `<h3>SD Card Status</h3>`
**Purpose:** Show mounted state, space, and filesystem info.
**Fields:**
- **Mounted State Indicator:** Shows if SD card is mounted and accessible.
- **Total Space:** Displays total SD card capacity.
- **Free Space:** Shows available space for files.
- **Filesystem Type:** Indicates format (e.g., FAT32).
- **Refresh Button:** Reloads SD card status.
**Step-by-Step Usage:**
1. Confirm SD card is mounted and accessible.
2. Review total and free space before uploading files.
3. Click Refresh to update status after file operations.
**Troubleshooting:**
- If SD card is not mounted, check hardware and format.
- If space is low, delete old files or expand capacity.
**Advanced Tips:**
- Use high-capacity cards for large databases and logs.
- Format card to recommended filesystem for best compatibility.
**Example Scenario:**
You add a new SD card; check status and free space before uploading data.

#### File Browser Card
**Header:** `<h3>File Browser</h3>`
**Purpose:** Browse, upload, download, and delete files on the SD card.
**Fields:**
- **File List:** Table of all files on SD card, showing name, size, and date modified.
- **Upload Button:** Opens file picker to upload new files.
- **Download Button:** Downloads selected file to your computer.
- **Delete Button:** Removes selected file from SD card.
- **Refresh Button:** Reloads file list to show latest changes.
**Step-by-Step Usage:**
1. Use File List to browse SD card contents.
2. Click Upload to add new files (e.g., logs, databases).
3. Select a file and click Download to save a backup.
4. Select a file and click Delete to remove it.
5. Click Refresh after file operations to update the list.
**Troubleshooting:**
- If upload/download fails, check file size and browser compatibility.
- If delete fails, verify file permissions and free space.
**Advanced Tips:**
- Use SD card for large databases and log files to save internal flash space.
- Regularly backup SD card files to your computer.
**Example Scenario:**
You want to archive log files; download them from SD card and delete old entries.

---

## 18. Advanced Usage Scenarios & Best Practices

This section provides practical, real-world scenarios, advanced tips, and best practices for every major page, card, and button in the ESP32 MMDVM Hotspot web interface. Use these examples to maximize reliability, performance, and ease of use in the field.

### Main Page
- **Scenario:** After a firmware update, verify all modes and services are enabled and healthy. Use the Welcome Card for a feature summary, and the Services Card to check MQTT and VPN status.
- **Best Practice:** Always complete the First-time Setup Card after resets or updates. Use the Last 15 Calls Card to audit recent activity and spot anomalies.

### System Info Page
- **Scenario:** Device is unstable after enabling new services. Use Memory and Task Stack Usage Cards to monitor resource usage. Expand All Tasks for deep debugging.
- **Best Practice:** Regularly check Storage Card before OTA updates. Use Software Card to track uptime and reset reasons for long-term stability.

### System Hardware Page
- **Scenario:** Upgrading to a new OLED or SD card. Use OLED Settings and SD Card Settings Cards to match hardware specs. Save All & Reboot after multiple changes.
- **Best Practice:** Document all GPIO assignments for future troubleshooting. Use Reset buttons before swapping hardware.

### System Status Page
- **Scenario:** Moving device to a new location. Use WiFi Status Card to scan and connect to new networks. Check Ethernet Status for wired deployments.
- **Best Practice:** Use Station Information Card to confirm callsign and DMR ID before transmitting. Monitor WireGuard Status for secure remote access.

### System Files Page
- **Scenario:** Archiving logs and updating databases. Use SD Card Browser and LittleFS File Browser Cards to backup and manage files. Use Bootlogos Installer for branding.
- **Best Practice:** Regularly backup configuration and log files. Delete old files to maintain free space.

### Serial Monitor Page
- **Scenario:** Diagnosing modem errors. Use Log Viewer Card to copy logs for support. Use Status Bar Card to check UART health and reconnect if needed.
- **Best Practice:** Save logs before firmware updates. Use Clear to reset viewer for new troubleshooting sessions.

### Mode Pages
- **Scenario:** Testing new digital voice mode. Use Mode Status Card to enable/disable modes. Use Settings Cards to configure IDs, frequencies, and network servers.
- **Best Practice:** Save configuration before firmware updates. Use Reset to revert to defaults if issues arise.

### Service Pages
- **Scenario:** Setting up automation and secure access. Use MQTT Status Card to connect to broker and monitor events. Use WireGuard Status Card for VPN setup.
- **Best Practice:** Use Admin Actions Card for factory reset and service restarts after major changes. Regularly backup SD card files.

### Card & Button Best Practices
- **Save:** Always click Save after changing any configuration. Confirm changes are applied before rebooting.
- **Reset:** Use Reset before troubleshooting persistent issues or after hardware swaps.
- **Connect/Disconnect:** Use Connect after entering credentials; Disconnect before changing sensitive settings.
- **Upload/Download/Delete:** Backup files before deleting. Use Download for regular archiving.
- **Scan/Refresh:** Scan for networks before connecting; Refresh cards after file or configuration changes.
- **Reboot/Restart:** Use after major updates or troubleshooting. Prefer Restart Services for quick recovery.
- **Copy/Clear:** Copy logs for support; Clear before new diagnostics.
- **Install/Rollback:** Use Install for new bootlogos/firmware; Rollback after failed updates.

### Field Tips
- **Document all configuration changes and hardware assignments.**
- **Regularly backup files and configuration before updates or resets.**
- **Monitor color-coded indicators for early warning of issues.**
- **Use advanced settings for custom routing, automation, and remote access.**
- **Consult source file mapping in the Appendix for troubleshooting and customization.**

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


Screenshots for each page will be added in the next revision.