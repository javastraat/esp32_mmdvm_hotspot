# MMDVM Modem Web Flasher - Implementation Plan

## Overview
Add WiFi and web interface to the MMDVM modem flasher, allowing firmware updates via:
1. **File Upload** - Upload .bin file through web interface
2. **URL Download** - Enter URL to download firmware (like ESP32 OTA updates)
3. **Serial Console** - Keep existing serial command interface

## Architecture

### Core Components
```
modem-flasher-web.ino
├── WiFi Setup (Station + AP modes)
├── Web Server (AsyncWebServer or WebServer)
├── File Upload Handler
├── URL Download Handler
├── Progress/Status API
├── Serial Console (existing functionality)
└── STM32 Bootloader Functions (existing)
```

### WiFi Configuration
```cpp
// Try to connect to configured WiFi
// If fails -> Start AP mode
const char* WIFI_SSID = "TechInc";
const char* WIFI_PASSWORD = "itoldyoualready";
const char* AP_SSID = "MMDVM-Flasher";
const char* AP_PASSWORD = "flash1234";
```

### Web Interface Pages

#### 1. Main Page (`/`)
```html
┌────────────────────────────────────┐
│  MMDVM Modem Firmware Flasher     │
├────────────────────────────────────┤
│ Status:                            │
│  • Modem Version: v1.6.1           │
│  • Bootloader: Inactive            │
│  • WiFi: Connected (192.168.1.100) │
├────────────────────────────────────┤
│ Option 1: Upload Firmware File    │
│  [Choose File] [Upload & Flash]   │
├────────────────────────────────────┤
│ Option 2: Flash from URL           │
│  https://github.com/.../fw.bin     │
│  [Download & Flash]                │
├────────────────────────────────────┤
│ Quick Actions:                     │
│  [Enter Bootloader] [Exit] [Read] │
├────────────────────────────────────┤
│ Progress: ██████░░░░ 60%          │
│                                    │
│ Log:                               │
│  10:15:23: Entering bootloader...  │
│  10:15:24: Bootloader active       │
│  10:15:25: Erasing flash...        │
└────────────────────────────────────┘
```

#### 2. Upload Handler (`/upload`)
- Accepts multipart/form-data
- Saves to SPIFFS as `/firmware.bin`
- Triggers flash process
- Shows progress

#### 3. URL Download Handler (`/flashurl`)
- Downloads firmware from URL
- Saves to SPIFFS
- Triggers flash process
- Progress updates via WebSocket or polling

#### 4. Status API (`/status`)
```json
{
  "modemVersion": "MMDVM_HS_Hat-v1.6.1",
  "inBootloader": false,
  "flashProgress": 60,
  "flashStatus": "Writing firmware...",
  "wifiStatus": "connected",
  "ipAddress": "192.168.1.100"
}
```

#### 5. Action API (`/api?action=X`)
- `enterBootloader` - Enter STM32 bootloader mode
- `exitBootloader` - Exit bootloader, reset to normal
- `readVersion` - Read current modem firmware version
- `getChipID` - Get STM32 chip ID
- `eraseFlash` - Erase flash memory
- `flashStatus` - Get current flash operation status

## Web Flash Process Flow

### Upload Method
```
1. User uploads .bin file
2. Save to SPIFFS (/firmware.bin)
3. Enter bootloader mode
4. Erase STM32 flash
5. Write firmware (show progress)
6. Verify (optional)
7. Exit bootloader
8. Read new version
```

### URL Method
```
1. User enters firmware URL
2. Download to SPIFFS with progress
3. Enter bootloader mode
4. Erase STM32 flash
5. Write firmware (show progress)
6. Exit bootloader
7. Read new version
```

## Code Structure

### File Organization
```
modem-flasher-web/
├── modem-flasher-web.ino       # Main sketch
├── web_handlers.h              # Web page handlers
├── stm32_flash.h               # STM32 bootloader functions
├── wifi_config.h               # WiFi configuration
└── data/                       # SPIFFS data
    └── index.html              # Optional: serve from SPIFFS
```

### Key Functions

#### WiFi Setup
```cpp
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi failed, starting AP");
    WiFi.softAP(AP_SSID, AP_PASSWORD);
  }

  Serial.println("IP: " + WiFi.localIP().toString());
}
```

#### File Upload Handler
```cpp
void handleUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    File file = SPIFFS.open("/firmware.bin", "w");
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    file.write(upload.buf, upload.currentSize);
  }
  else if (upload.status == UPLOAD_FILE_END) {
    file.close();
    flashFirmwareFromFile("/firmware.bin");
  }
}
```

#### URL Download Handler
```cpp
void handleFlashFromURL() {
  String url = server.arg("url");

  HTTPClient http;
  http.begin(url);

  if (http.GET() == HTTP_CODE_OK) {
    File file = SPIFFS.open("/firmware.bin", "w");
    WiFiClient* stream = http.getStreamPtr();

    while (stream->available()) {
      file.write(stream->read());
    }

    file.close();
    flashFirmwareFromFile("/firmware.bin");
  }
}
```

#### Flash with Progress
```cpp
bool flashFirmwareFromFile(String filename) {
  // Enter bootloader
  flashStatus = "Entering bootloader...";
  flashProgress = 5;
  enterBootloader();

  // Erase
  flashStatus = "Erasing flash...";
  flashProgress = 20;
  eraseFlashMemory();

  // Write
  File file = SPIFFS.open(filename, "r");
  size_t fileSize = file.size();
  uint32_t address = 0x08000000;

  while (file.available()) {
    size_t chunk = file.read(buffer, 256);
    writeMemory(address, buffer, chunk);
    address += chunk;

    flashProgress = 20 + (address * 60 / fileSize);
    flashStatus = "Writing: " + String(address) + " / " + String(fileSize);
  }

  // Exit bootloader
  flashProgress = 90;
  flashStatus = "Resetting...";
  exitBootloader();

  flashProgress = 100;
  flashStatus = "Complete!";
  return true;
}
```

## JavaScript for Real-Time Updates

```javascript
// Poll status every 2 seconds
setInterval(() => {
  fetch('/status')
    .then(r => r.json())
    .then(data => {
      document.getElementById('progress').style.width = data.flashProgress + '%';
      document.getElementById('status').textContent = data.flashStatus;
      document.getElementById('modemVersion').textContent = data.modemVersion;
    });
}, 2000);
```

## Firmware URL Examples

Users can flash firmware directly from URLs:
```
https://github.com/juribeparada/MMDVM_HS/releases/download/v1.6.1/mmdvm_hs_hat_fw.bin
https://yourserver.com/firmware/latest.bin
http://192.168.1.50/modem-firmware.bin
```

## Security Considerations

1. **Authentication** - Add basic auth for web interface
2. **HTTPS** - Use HTTPS for URL downloads if possible
3. **File Validation** - Check file size, magic bytes
4. **Rate Limiting** - Prevent spam uploads
5. **Timeout** - Set reasonable timeouts for downloads

## Next Steps

1. **Phase 1**: Basic web interface with file upload
2. **Phase 2**: Add URL download capability
3. **Phase 3**: Add real-time progress via WebSocket
4. **Phase 4**: Add authentication
5. **Phase 5**: Integrate with main ESP32 MMDVM web interface

## Integration with Main Project

The web flasher can be:
1. **Standalone** - Separate sketch for flashing only
2. **Integrated** - Add `/modem-flash` page to main web interface
3. **Hybrid** - Use same WiFi config, add pages to existing server

Recommended: **Integrated** approach
- Add to existing web/pages/ directory
- Reuse WiFi configuration from config.h
- Add menu item in main navigation
- Share authentication with admin pages

## File: `web/pages/modem_flash.h`

```cpp
void handleModemFlash() {
  if (!checkAuthentication()) return;

  String html = getCommonCSS();
  html += getNavigation("modem");
  html += "<h2>Modem Firmware Flasher</h2>";
  // ... flash interface ...
  html += getFooter();

  server.send(200, "text/html", html);
}
```

## Advantages of Web Interface

✅ No need for serial console access
✅ Can flash from anywhere on network
✅ Download firmware directly from GitHub
✅ Visual progress indication
✅ Mobile-friendly interface
✅ Easier for non-technical users
✅ Can integrate with existing admin panel

## Testing Plan

1. Test file upload (< 128KB firmware)
2. Test URL download from GitHub
3. Test with bad URL (error handling)
4. Test with corrupted firmware (error handling)
5. Test progress updates
6. Test bootloader entry/exit
7. Test version reading after flash
8. Test concurrent access
9. Test AP mode fallback
10. Test mobile responsiveness
