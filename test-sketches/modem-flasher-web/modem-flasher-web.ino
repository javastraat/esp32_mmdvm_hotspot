/*
 * MMDVM Modem Firmware Flasher (Web-Enabled Standalone Version)
 *
 * Features:
 * - WiFi connectivity (Station + AP fallback)
 * - Web interface for firmware upload
 * - Download firmware from URL
 * - Serial console (keeps all existing functionality)
 * - Real-time progress updates
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

// ===== WiFi Configuration (change these!) =====
const char* WIFI_SSID = "TechInc";
const char* WIFI_PASSWORD = "itoldyoualready";
const char* AP_SSID = "MMDVM-Flasher";
const char* AP_PASSWORD = "flash1234";

// ===== Pin Configuration =====
#define MMDVM_RX_PIN 44
#define MMDVM_TX_PIN 43
#define MMDVM_BOOT0_PIN 4
#define MMDVM_RESET_PIN 13

// ===== MMDVM Protocol =====
#define MMDVM_FRAME_START 0xE0
#define CMD_GET_VERSION   0x00

// ===== STM32 Bootloader Protocol =====
#define STM32_SYNC_BYTE   0x7F
#define STM32_ACK         0x79
#define STM32_NACK        0x1F

// STM32 Commands
#define STM32_CMD_GET     0x00
#define STM32_CMD_GID     0x02
#define STM32_CMD_RM      0x11
#define STM32_CMD_WM      0x31
#define STM32_CMD_EE      0x44

// Flash Memory
#define FLASH_START_ADDR  0x08000000
#define MAX_WRITE_SIZE    256

// ===== Global Objects =====
WebServer server(80);
HardwareSerial MMDVMSerial(2);
HardwareSerial WakeupSerial(1);

// ===== Global State =====
bool inBootloaderMode = false;
bool wifiConnected = false;
String modemFirmwareVersion = "Unknown";
String flashStatus = "";
int flashProgress = 0;
bool flashInProgress = false;

// Streaming upload state
uint32_t uploadAddress = FLASH_START_ADDR;
size_t uploadTotalSize = 0;
size_t uploadBytesWritten = 0;
uint8_t uploadBuffer[MAX_WRITE_SIZE];
size_t uploadBufferPtr = 0;

// ===== Forward Declarations =====
void setupWiFi();
void setupWebServer();
void handleRoot();
void handleUpload();
void handleFlashURL();
void handleStatus();
void handleAPI();
bool eraseFlashMemory();

// STM32 functions
void enterBootloader();
void exitBootloader();
void readModemFirmwareVersion();
String testBootloaderConnection();
String getChipID();
String getBootloaderVersion();
bool syncBootloader();
bool sendCommand(uint8_t cmd);
bool waitForAck(const char* context);
bool writeMemory(uint32_t address, uint8_t* data, uint16_t length);
void hardwareResetToBootloader();
void hardwareResetToNormal();

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n================================================");
  Serial.println("  MMDVM MODEM FIRMWARE FLASHER (Web Edition)");
  Serial.println("================================================\n");

  // Initialize control pins
  pinMode(MMDVM_BOOT0_PIN, OUTPUT);
  pinMode(MMDVM_RESET_PIN, OUTPUT);
  digitalWrite(MMDVM_BOOT0_PIN, LOW);
  digitalWrite(MMDVM_RESET_PIN, HIGH);
  Serial.println("[OK] GPIO pins initialized");

  // No SPIFFS needed - we stream firmware directly to modem
  Serial.println("[OK] Firmware flashing uses direct streaming (no SPIFFS required)");

  // Setup WiFi and Web Server
  setupWiFi();
  setupWebServer();

  Serial.println("\n[READY] System ready!");
  Serial.println("Access web interface to flash firmware");
  Serial.println("Or use serial console for manual control\n");

  // Read modem firmware version automatically on boot
  Serial.println("[Boot] Reading modem firmware version...\n");
  readModemFirmwareVersion();
  Serial.println("\n[Boot] Initial setup complete!");
}

void loop() {
  server.handleClient();
  delay(10);
}

// ===== WiFi Setup =====
void setupWiFi() {
  Serial.println("\n[WiFi] Connecting to " + String(WIFI_SSID) + "...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\n[WiFi] Connected!");
    Serial.println("[WiFi] IP Address: " + WiFi.localIP().toString());
    Serial.println("[WiFi] Web interface: http://" + WiFi.localIP().toString());
  } else {
    Serial.println("\n[WiFi] Connection failed, starting AP mode...");
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.println("[AP] SSID: " + String(AP_SSID));
    Serial.println("[AP] Password: " + String(AP_PASSWORD));
    Serial.println("[AP] IP: " + WiFi.softAPIP().toString());
    Serial.println("[AP] Web interface: http://" + WiFi.softAPIP().toString());
  }
}

// ===== Web Server Setup =====
void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/upload", HTTP_POST,
    []() { server.send(200, "text/plain", flashStatus); },
    handleUpload
  );
  server.on("/flashurl", HTTP_POST, handleFlashURL);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/api", HTTP_GET, handleAPI);

  server.begin();
  Serial.println("[Web] Server started");
}

// ===== Web Handlers =====
void handleRoot() {
  String html = R"(<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <title>MMDVM Modem Flasher</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: Arial, sans-serif; background: #f0f2f5; }
    .container { max-width: 900px; margin: 20px auto; padding: 20px; }
    .card { background: white; border-radius: 8px; padding: 20px; margin: 15px 0; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }
    h1 { color: #1a73e8; margin-bottom: 10px; }
    h2 { color: #333; margin: 15px 0 10px 0; font-size: 18px; }
    .status-box { background: #e8f0fe; border-left: 4px solid #1a73e8; padding: 15px; margin: 15px 0; }
    .status-item { margin: 8px 0; }
    .status-label { font-weight: bold; color: #555; }
    .status-value { color: #1a73e8; }
    .section { margin: 20px 0; }
    button, input[type="submit"] { background: #1a73e8; color: white; padding: 12px 24px; border: none; border-radius: 4px; cursor: pointer; font-size: 14px; margin: 5px; }
    button:hover, input[type="submit"]:hover { background: #1557b0; }
    button:disabled { background: #ccc; cursor: not-allowed; }
    input[type="file"], input[type="text"] { padding: 10px; border: 2px solid #ddd; border-radius: 4px; width: 100%; margin: 10px 0; }
    .progress-container { background: #e0e0e0; border-radius: 4px; margin: 15px 0; height: 30px; position: relative; display: none; }
    .progress-bar { background: #4caf50; height: 100%; border-radius: 4px; transition: width 0.3s; }
    .progress-text { position: absolute; width: 100%; text-align: center; line-height: 30px; color: #333; font-weight: bold; }
    .log { background: #1e1e1e; color: #00ff00; padding: 15px; border-radius: 4px; height: 250px; overflow-y: auto; font-family: 'Courier New', monospace; font-size: 13px; margin: 15px 0; }
    .log-entry { margin: 3px 0; }
    .btn-group { margin: 15px 0; }
    .success { color: #4caf50; }
    .error { color: #f44336; }
    .warning { color: #ff9800; }
  </style>
</head>
<body>
  <div class='container'>
    <div class='card'>
      <h1>MMDVM Modem Firmware Flasher</h1>
      <p>Web interface for flashing STM32 modem firmware</p>
    </div>

    <div class='card'>
      <h2>System Status</h2>
      <div class='status-box'>
        <div class='status-item'>
          <span class='status-label'>Modem Firmware:</span>
          <span class='status-value' id='modemVersion'>)" + modemFirmwareVersion + R"(</span>
        </div>
        <div class='status-item'>
          <span class='status-label'>Bootloader Mode:</span>
          <span class='status-value' id='bootMode'>)" + String(inBootloaderMode ? "Active" : "Inactive") + R"(</span>
        </div>
        <div class='status-item'>
          <span class='status-label'>WiFi Status:</span>
          <span class='status-value'>)" + String(wifiConnected ? "Connected" : "AP Mode") + R"(</span>
        </div>
        <div class='status-item'>
          <span class='status-label'>Flash Status:</span>
          <span class='status-value' id='flashStat'>Idle</span>
        </div>
      </div>
    </div>

    <div class='card'>
      <h2>Option 1: Upload Firmware File</h2>
      <p>Upload a .bin firmware file from your computer</p>
      <form id='uploadForm' enctype='multipart/form-data'>
        <input type='file' name='firmware' id='fileInput' accept='.bin' required>
        <button type='submit' id='uploadBtn'>Upload & Flash Firmware</button>
      </form>
    </div>

    <div class='card'>
      <h2>Option 2: Flash from URL</h2>
      <p>Download firmware from a URL (e.g., GitHub releases)</p>
      <form id='urlForm'>
        <select id='firmwareSelect' style='width: 100%; padding: 10px; margin-bottom: 10px; border: 1px solid #ddd; border-radius: 4px;'>
          <option value=''>Select firmware version...</option>
          <option value='https://github.com/javastraat/esp32_mmdvm_hotspot/raw/refs/heads/main/firmware/mmdvm/mmdvm_hs_hat_fw.bin'>Single MMDVM Modem v1.6.1</option>
          <option value='https://github.com/javastraat/esp32_mmdvm_hotspot/raw/refs/heads/main/firmware/mmdvm/mmdvm_hs_dual_hat_fw.bin'>Dual MMDVM Modem v1.6.1</option>
          <option value='https://github.com/javastraat/esp32_mmdvm_hotspot/raw/refs/heads/main/firmware/mmdvm/generic_gpio_fw152.bin'>Single MMDVM Modem v1.5.2</option>
          <option value='custom'>Enter custom URL...</option>
        </select>
        <input type='text' id='urlInput' placeholder='Enter custom firmware URL...' style='display: none; width: 100%; padding: 10px; margin-bottom: 10px; border: 1px solid #ddd; border-radius: 4px;'>
        <button type='submit' id='urlBtn'>Download & Flash</button>
      </form>
    </div>

    <div class='card'>
      <h2>Quick Actions</h2>
      <div class='btn-group'>
        <button onclick='readVersion()'>1. Read Modem Version</button>
        <button onclick='enterBoot()'>2. Enter Bootloader</button>
        <button onclick='exitBoot()'>3. Exit Bootloader</button>
      </div>
      <div class='btn-group'>
        <button onclick='testBootloader()'>4. Test Bootloader</button>
        <button onclick='getChipID()'>5. Get Chip ID</button>
        <button onclick='getBootloaderVersion()'>6. Get Bootloader Version</button>
      </div>
    </div>

    <div class='card'>
      <div class='progress-container' id='progressContainer'>
        <div class='progress-bar' id='progressBar'></div>
        <div class='progress-text' id='progressText'>0%</div>
      </div>

      <h2>Activity Log</h2>
      <div class='log' id='log'></div>
    </div>
  </div>

  <script>
    function log(msg, type = 'info') {
      const logDiv = document.getElementById('log');
      const time = new Date().toLocaleTimeString();
      const className = type === 'error' ? 'error' : type === 'success' ? 'success' : type === 'warning' ? 'warning' : '';
      logDiv.innerHTML += '<div class="log-entry ' + className + '">[' + time + '] ' + msg + '</div>';
      logDiv.scrollTop = logDiv.scrollHeight;
    }

    let lastStatus = '';
    let lastProgress = 0;

    function updateStatus() {
      fetch('/status')
        .then(r => r.json())
        .then(data => {
          document.getElementById('modemVersion').textContent = data.modemVersion;
          document.getElementById('bootMode').textContent = data.inBootloader ? 'Active' : 'Inactive';
          document.getElementById('flashStat').textContent = data.flashStatus || 'Idle';

          // Log status changes to activity log
          if (data.flashStatus && data.flashStatus !== lastStatus && data.flashStatus !== 'Idle' && data.flashStatus !== '') {
            let logType = 'info';
            if (data.flashStatus.startsWith('ERROR')) {
              logType = 'error';
            } else if (data.flashStatus.includes('complete')) {
              logType = 'success';
            }
            log(data.flashStatus, logType);
            lastStatus = data.flashStatus;
          }

          if (data.flashProgress > 0) {
            document.getElementById('progressContainer').style.display = 'block';
            document.getElementById('progressBar').style.width = data.flashProgress + '%';
            document.getElementById('progressText').textContent = data.flashProgress + '%';
            lastProgress = data.flashProgress;
          } else if (!data.flashInProgress && lastProgress === 100) {
            // Keep showing 100% for completed flashes
            document.getElementById('progressBar').style.width = '100%';
            document.getElementById('progressText').textContent = '100%';
          }
        })
        .catch(e => console.error('Status update failed:', e));
    }

    // File upload handler
    document.getElementById('uploadForm').onsubmit = function(e) {
      e.preventDefault();
      const fileInput = document.getElementById('fileInput');
      const file = fileInput.files[0];

      if (!file) {
        log('Please select a file', 'error');
        return;
      }

      log('Uploading: ' + file.name + ' (' + file.size + ' bytes)');
      document.getElementById('uploadBtn').disabled = true;

      const formData = new FormData();
      formData.append('firmware', file);

      fetch('/upload', {
        method: 'POST',
        body: formData
      })
      .then(r => r.text())
      .then(msg => {
        log(msg, 'success');
        document.getElementById('uploadBtn').disabled = false;
        fileInput.value = '';
      })
      .catch(e => {
        log('Upload failed: ' + e, 'error');
        document.getElementById('uploadBtn').disabled = false;
      });
    };

    // Firmware select handler
    document.getElementById('firmwareSelect').onchange = function() {
      const select = document.getElementById('firmwareSelect');
      const urlInput = document.getElementById('urlInput');

      if (select.value === 'custom') {
        urlInput.style.display = 'block';
        urlInput.required = true;
        urlInput.value = '';
      } else if (select.value === '') {
        urlInput.style.display = 'none';
        urlInput.required = false;
      } else {
        urlInput.style.display = 'none';
        urlInput.required = false;
        urlInput.value = select.value;
      }
    };

    // URL download handler
    document.getElementById('urlForm').onsubmit = function(e) {
      e.preventDefault();
      const select = document.getElementById('firmwareSelect');
      const urlInput = document.getElementById('urlInput');
      let url = '';

      if (select.value === 'custom') {
        url = urlInput.value;
      } else if (select.value !== '') {
        url = select.value;
      } else {
        log('Please select a firmware version', 'error');
        return;
      }

      log('Downloading from: ' + url);
      document.getElementById('urlBtn').disabled = true;

      fetch('/flashurl', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'url=' + encodeURIComponent(url)
      })
      .then(r => r.text())
      .then(msg => {
        log(msg, 'success');
        document.getElementById('urlBtn').disabled = false;
      })
      .catch(e => {
        log('Download failed: ' + e, 'error');
        document.getElementById('urlBtn').disabled = false;
      });
    };

    // API functions
    function readVersion() {
      log('Reading modem version...');
      fetch('/api?action=readVersion')
        .then(r => r.text())
        .then(msg => log(msg, 'info'));
    }

    function enterBoot() {
      log('Entering bootloader mode...');
      fetch('/api?action=enterBootloader')
        .then(r => r.text())
        .then(msg => log(msg, 'info'));
    }

    function exitBoot() {
      log('Exiting bootloader mode...');
      fetch('/api?action=exitBootloader')
        .then(r => r.text())
        .then(msg => log(msg, 'info'));
    }

    function testBootloader() {
      log('Testing bootloader connection...');
      fetch('/api?action=testBootloader')
        .then(r => r.text())
        .then(msg => log(msg, 'info'));
    }

    function getChipID() {
      log('Getting STM32 chip ID...');
      fetch('/api?action=getChipID')
        .then(r => r.text())
        .then(msg => log(msg, 'info'));
    }

    function getBootloaderVersion() {
      log('Getting bootloader version...');
      fetch('/api?action=getBootloaderVersion')
        .then(r => r.text())
        .then(msg => log(msg, 'info'));
    }

    // Update status every 2 seconds
    setInterval(updateStatus, 2000);
    log('Web interface ready', 'success');
  </script>
</body>
</html>)";

  server.send(200, "text/html; charset=UTF-8", html);
}

void handleUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    flashStatus = "Preparing: " + String(upload.filename);
    flashProgress = 5;
    Serial.println("[Upload] Starting: " + String(upload.filename));

    // Note: totalSize may not be available yet in UPLOAD_FILE_START
    // We'll get the actual size during UPLOAD_FILE_WRITE
    uploadTotalSize = 0;  // Will be updated as we receive data
    uploadBytesWritten = 0;
    uploadBufferPtr = 0;

    // Enter bootloader mode
    if (!inBootloaderMode) {
      flashStatus = "Entering bootloader mode...";
      flashProgress = 10;
      enterBootloader();
      delay(500);
    }

    if (!inBootloaderMode) {
      flashStatus = "ERROR: Could not enter bootloader";
      Serial.println("[Upload] " + flashStatus);
      return;
    }

    // Sync with bootloader
    flashStatus = "Syncing with STM32 bootloader...";
    flashProgress = 12;
    Serial.println("[Upload] Syncing with bootloader...");
    if (!syncBootloader()) {
      flashStatus = "ERROR: Bootloader sync failed";
      Serial.println("[Upload] " + flashStatus);
      return;
    }

    // Erase flash
    flashStatus = "Erasing STM32 flash memory...";
    flashProgress = 20;
    Serial.println("[Upload] Erasing flash...");
    if (!eraseFlashMemory()) {
      flashStatus = "ERROR: Erase failed";
      Serial.println("[Upload] " + flashStatus);
      return;
    }

    // Initialize streaming state
    uploadAddress = FLASH_START_ADDR;
    flashInProgress = true;
    flashStatus = "Writing firmware to STM32...";
    flashProgress = 30;
    Serial.println("[Upload] Starting write...");
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    // Update total size as we receive data
    if (uploadTotalSize == 0 && upload.totalSize > 0) {
      uploadTotalSize = upload.totalSize;
      Serial.printf("[Upload] File size: %d bytes\n", uploadTotalSize);
    }

    // Stream chunks directly to STM32
    for (size_t i = 0; i < upload.currentSize; i++) {
      uploadBuffer[uploadBufferPtr++] = upload.buf[i];

      // When buffer is full (256 bytes), write to STM32
      if (uploadBufferPtr >= MAX_WRITE_SIZE) {
        if (!writeMemory(uploadAddress, uploadBuffer, MAX_WRITE_SIZE)) {
          flashStatus = "ERROR: Write failed at 0x" + String(uploadAddress, HEX);
          Serial.println("[Upload] " + flashStatus);
          flashInProgress = false;
          return;
        }

        uploadAddress += MAX_WRITE_SIZE;
        uploadBytesWritten += MAX_WRITE_SIZE;
        uploadBufferPtr = 0;

        // Update progress (30% to 90%)
        if (uploadTotalSize > 0) {
          flashProgress = 30 + (uploadBytesWritten * 60 / uploadTotalSize);
        }

        if (uploadBytesWritten % (16 * 256) == 0) {
          if (uploadTotalSize > 0) {
            Serial.printf("[Upload] Progress: %d / %d (%.1f%%)\n",
              uploadBytesWritten, uploadTotalSize, (uploadBytesWritten * 100.0) / uploadTotalSize);
          } else {
            Serial.printf("[Upload] Progress: %d bytes written\n", uploadBytesWritten);
          }
        }
      }
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    // Write any remaining bytes (pad to 256 bytes)
    if (uploadBufferPtr > 0) {
      memset(uploadBuffer + uploadBufferPtr, 0xFF, MAX_WRITE_SIZE - uploadBufferPtr);
      if (!writeMemory(uploadAddress, uploadBuffer, MAX_WRITE_SIZE)) {
        flashStatus = "ERROR: Final write failed";
        Serial.println("[Upload] " + flashStatus);
        flashInProgress = false;
        return;
      }
      uploadBytesWritten += uploadBufferPtr;
    }

    Serial.printf("[Upload] Complete! %d bytes written\n", uploadBytesWritten);

    // Exit bootloader
    flashStatus = "Resetting modem to normal mode...";
    flashProgress = 92;
    exitBootloader();
    delay(1000);

    // Read new version
    flashStatus = "Verifying new firmware version...";
    flashProgress = 95;
    readModemFirmwareVersion();

    flashStatus = "Flash complete! New version: " + modemFirmwareVersion;
    flashProgress = 100;
    flashInProgress = false;
    Serial.println("[Upload] Flash complete!");
  }
}

void handleFlashURL() {
  if (!server.hasArg("url")) {
    server.send(400, "text/plain", "ERROR: Missing URL parameter");
    return;
  }

  String url = server.arg("url");

  // Immediate response to start process
  server.send(202, "text/plain", "Starting flash from URL...");

  flashStatus = "Connecting to server...";
  flashProgress = 5;
  Serial.println("[URL] Starting download: " + url);

  HTTPClient http;
  http.begin(url);
  http.setTimeout(30000); // 30 second timeout
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS); // Follow GitHub redirects

  flashStatus = "Downloading firmware...";
  flashProgress = 8;
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    // Enter bootloader mode
    if (!inBootloaderMode) {
      flashStatus = "Entering bootloader mode...";
      flashProgress = 10;
      enterBootloader();
      delay(500);
    }

    if (!inBootloaderMode) {
      flashStatus = "ERROR: Could not enter bootloader";
      Serial.println("[URL] " + flashStatus);
      http.end();
      return;
    }

    // Sync with bootloader
    flashStatus = "Syncing with STM32 bootloader...";
    flashProgress = 12;
    Serial.println("[URL] Syncing with bootloader...");
    if (!syncBootloader()) {
      flashStatus = "ERROR: Bootloader sync failed";
      Serial.println("[URL] " + flashStatus);
      http.end();
      return;
    }

    // Erase flash
    flashStatus = "Erasing STM32 flash memory...";
    flashProgress = 15;
    Serial.println("[URL] Erasing flash...");
    if (!eraseFlashMemory()) {
      flashStatus = "ERROR: Erase failed";
      Serial.println("[URL] " + flashStatus);
      http.end();
      return;
    }

    // Stream and write directly to STM32
    flashStatus = "Writing firmware to STM32...";
    flashProgress = 25;
    flashInProgress = true;

    WiFiClient* stream = http.getStreamPtr();
    int totalLen = http.getSize();
    uint32_t address = FLASH_START_ADDR;
    size_t bytesWritten = 0;
    uint8_t buffer[MAX_WRITE_SIZE];
    size_t bufferPtr = 0;

    Serial.println("[URL] Streaming to STM32...");

    while (http.connected() && (totalLen > 0 || totalLen == -1)) {
      size_t available = stream->available();
      if (available) {
        // Read byte by byte into buffer
        while (available > 0 && bufferPtr < MAX_WRITE_SIZE) {
          buffer[bufferPtr++] = stream->read();
          available--;
          if (totalLen > 0) totalLen--;
        }

        // When buffer is full, write to STM32
        if (bufferPtr >= MAX_WRITE_SIZE) {
          if (!writeMemory(address, buffer, MAX_WRITE_SIZE)) {
            flashStatus = "ERROR: Write failed at 0x" + String(address, HEX);
            Serial.println("[URL] " + flashStatus);
            flashInProgress = false;
            server.send(500, "text/plain", flashStatus);
            http.end();
            return;
          }

          address += MAX_WRITE_SIZE;
          bytesWritten += MAX_WRITE_SIZE;
          bufferPtr = 0;

          // Update progress (25% to 90%)
          flashProgress = 25 + (bytesWritten * 65 / (bytesWritten + max(totalLen, 0)));

          if (bytesWritten % (16 * 256) == 0) {
            Serial.printf("[URL] Progress: %d bytes written\n", bytesWritten);
          }
        }
      }
      delay(1);
    }

    // Write any remaining bytes (pad to 256 bytes)
    if (bufferPtr > 0) {
      memset(buffer + bufferPtr, 0xFF, MAX_WRITE_SIZE - bufferPtr);
      if (!writeMemory(address, buffer, MAX_WRITE_SIZE)) {
        flashStatus = "ERROR: Final write failed";
        Serial.println("[URL] " + flashStatus);
        flashInProgress = false;
        server.send(500, "text/plain", flashStatus);
        http.end();
        return;
      }
      bytesWritten += bufferPtr;
    }

    Serial.printf("[URL] Complete! %d bytes written\n", bytesWritten);

    // Exit bootloader
    flashStatus = "Resetting modem to normal mode...";
    flashProgress = 92;
    exitBootloader();
    delay(1000);

    // Read new version
    flashStatus = "Verifying new firmware version...";
    flashProgress = 95;
    readModemFirmwareVersion();

    flashStatus = "Flash complete! New version: " + modemFirmwareVersion;
    flashProgress = 100;
    flashInProgress = false;
    Serial.println("[URL] Flash complete!");
  } else {
    flashStatus = "ERROR: HTTP " + String(httpCode);
    Serial.println("[URL] " + flashStatus);
    server.send(500, "text/plain", flashStatus);
  }

  http.end();
}

void handleStatus() {
  String json = "{";
  json += "\"modemVersion\":\"" + modemFirmwareVersion + "\",";
  json += "\"inBootloader\":" + String(inBootloaderMode ? "true" : "false") + ",";
  json += "\"flashProgress\":" + String(flashProgress) + ",";
  json += "\"flashStatus\":\"" + flashStatus + "\",";
  json += "\"flashInProgress\":" + String(flashInProgress ? "true" : "false") + ",";
  json += "\"lastLog\":\"\"";
  json += "}";

  server.send(200, "application/json", json);
}

void handleAPI() {
  if (!server.hasArg("action")) {
    server.send(400, "text/plain", "Missing action");
    return;
  }

  String action = server.arg("action");
  String result = "";

  if (action == "enterBootloader") {
    enterBootloader();
    result = inBootloaderMode ? "Bootloader active" : "Failed to enter bootloader";
  }
  else if (action == "exitBootloader") {
    exitBootloader();
    result = "Exited bootloader";
  }
  else if (action == "readVersion") {
    readModemFirmwareVersion();
    result = "Version: " + modemFirmwareVersion;
  }
  else if (action == "testBootloader") {
    result = testBootloaderConnection();
  }
  else if (action == "getChipID") {
    result = getChipID();
  }
  else if (action == "getBootloaderVersion") {
    result = getBootloaderVersion();
  }
  else {
    result = "Unknown action";
  }

  server.send(200, "text/plain", result);
}

// ===== Flash Functions =====

bool eraseFlashMemory() {
  Serial.println("[Erase] Erasing flash...");

  if (!sendCommand(STM32_CMD_EE)) {
    Serial.println("[Erase] Extended erase not supported");
    return false;
  }

  // Global erase: 0xFFFF
  MMDVMSerial.write(0xFF);
  MMDVMSerial.write(0xFF);
  MMDVMSerial.write(0x00);
  MMDVMSerial.flush();

  // Wait for erase (can take 10-30 seconds)
  unsigned long timeout = millis() + 60000;
  while (millis() < timeout) {
    if (MMDVMSerial.available()) {
      uint8_t response = MMDVMSerial.read();
      if (response == STM32_ACK) {
        Serial.println("[Erase] Complete!");
        return true;
      } else if (response == STM32_NACK) {
        Serial.println("[Erase] NACK received");
        return false;
      }
    }
    delay(100);
  }

  Serial.println("[Erase] Timeout");
  return false;
}

// ===== STM32 Bootloader Functions =====

void hardwareResetToBootloader() {
  Serial.println("[RESET] Executing hardware reset sequence (stm32flash compatible)...");

  // Stop all serial communication first
  MMDVMSerial.end();
  WakeupSerial.end();
  delay(100);

  // Step 1: Set BOOT0 HIGH (RPi GPIO20 = ESP32 GPIO4)
  Serial.println("  [1/4] Setting BOOT0 (GPIO4) HIGH");
  pinMode(MMDVM_BOOT0_PIN, OUTPUT);
  digitalWrite(MMDVM_BOOT0_PIN, HIGH);
  delay(100);

  // Step 2: Assert RESET LOW (RPi GPIO21 = ESP32 GPIO13)
  Serial.println("  [2/4] Asserting RESET (GPIO13) LOW");
  pinMode(MMDVM_RESET_PIN, OUTPUT);
  digitalWrite(MMDVM_RESET_PIN, LOW);
  delay(100);

  // Step 3: Release RESET HIGH (modem boots with BOOT0=HIGH -> bootloader)
  Serial.println("  [3/4] Releasing RESET (GPIO13) HIGH");
  digitalWrite(MMDVM_RESET_PIN, HIGH);
  delay(500);  // Give STM32 time to boot into bootloader

  Serial.println("  [4/4] Modem should now be in bootloader mode");
}

void hardwareResetToNormal() {
  Serial.println("[RESET] Resetting to normal firmware mode...");

  // Stop all serial communication
  MMDVMSerial.end();
  WakeupSerial.end();
  delay(100);

  // Step 1: Set BOOT0 LOW (normal mode)
  Serial.println("  [1/3] Setting BOOT0 (GPIO4) LOW");
  digitalWrite(MMDVM_BOOT0_PIN, LOW);
  delay(100);

  // Step 2: Assert RESET LOW
  Serial.println("  [2/3] Asserting RESET (GPIO13) LOW");
  digitalWrite(MMDVM_RESET_PIN, LOW);
  delay(100);

  // Step 3: Release RESET HIGH (modem boots with BOOT0=LOW -> normal firmware)
  Serial.println("  [3/3] Releasing RESET (GPIO13) HIGH");
  digitalWrite(MMDVM_RESET_PIN, HIGH);
  delay(500);

  Serial.println("  Modem should now be running normal firmware");
}

bool sendCommand(uint8_t cmd) {
  MMDVMSerial.write(cmd);
  MMDVMSerial.write(~cmd);
  MMDVMSerial.flush();
  return waitForAck("Command");
}

bool waitForAck(const char* context) {
  unsigned long timeout = millis() + 1000;

  while (millis() < timeout) {
    if (MMDVMSerial.available()) {
      uint8_t response = MMDVMSerial.read();
      if (response == STM32_ACK) return true;
      if (response == STM32_NACK) {
        Serial.println("[ERROR] " + String(context) + ": NACK");
        return false;
      }
    }
    delay(1);
  }

  Serial.println("[ERROR] " + String(context) + ": Timeout");
  return false;
}

bool syncBootloader() {
  while (MMDVMSerial.available()) MMDVMSerial.read();

  MMDVMSerial.write(STM32_SYNC_BYTE);
  MMDVMSerial.flush();

  return waitForAck("Sync");
}

void enterBootloader() {
  Serial.println("[ACTION] Entering STM32 bootloader mode...");
  Serial.println();

  // Use hardware reset sequence (stm32flash compatible)
  hardwareResetToBootloader();

  // Start serial at bootloader baud rate (8E1 parity)
  Serial.println("[STEP 5] Starting UART at bootloader speed (115200 baud, 8E1)...");
  // Note: STM32 bootloader uses EVEN parity
  MMDVMSerial.begin(115200, SERIAL_8E1, MMDVM_RX_PIN, MMDVM_TX_PIN);
  delay(100);

  Serial.println("[SUCCESS] Entered bootloader mode");
  Serial.println("[INFO] BOOT0 is HIGH, GPIO13 performed hardware reset");
  Serial.println("[INFO] Use option 4 to test bootloader connection");
  inBootloaderMode = true;
}

void exitBootloader() {
  Serial.println("[ACTION] Exiting bootloader mode...");
  Serial.println();

  // Use hardware reset sequence to return to normal mode
  hardwareResetToNormal();

  Serial.println("[SUCCESS] Exited bootloader mode");
  Serial.println("[INFO] Modem should now boot into normal firmware");
  Serial.println("[INFO] Use option 1 to verify firmware version");
  inBootloaderMode = false;
}

void readModemFirmwareVersion() {
  Serial.println("[ACTION] Reading modem firmware version...");
  Serial.println();

  if (inBootloaderMode) {
    Serial.println("[ERROR] Cannot read firmware version while in bootloader mode");
    Serial.println("[INFO] Use option 3 to exit bootloader mode first");
    modemFirmwareVersion = "Bootloader Mode";
    return;
  }

  // Start main MMDVM serial FIRST (like in main ino)
  Serial.println("[STEP 1] Starting main MMDVM serial (115200 baud)...");
  MMDVMSerial.begin(115200, SERIAL_8N1, MMDVM_RX_PIN, MMDVM_TX_PIN);
  delay(100);

  // Start keep-alive serial on GPIO 13 (CRITICAL - must stay active!)
  Serial.println("[STEP 2] Starting keep-alive serial on GPIO 13 (115200 baud)...");
  WakeupSerial.begin(115200, SERIAL_8N1, 1, MMDVM_RESET_PIN);  // RX=1, TX=13, same as main code
  delay(100);

  // Send wakeup commands and check for responses on BOTH serials
  Serial.println("[STEP 3] Sending wakeup burst (checking responses on wakeup serial)...");
  uint8_t wakeCmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};
  bool versionFromWakeup = false;
  uint8_t wakeRxBuffer[256];
  int wakeRxPtr = 0;

  for (int i = 0; i < 20 && !versionFromWakeup; i++) {
    WakeupSerial.write(wakeCmd, 3);
    WakeupSerial.flush();
    delay(100);  // Longer delay to allow response

    if (i % 5 == 0) {
      Serial.print(".");
    }

    // Check for response on wakeup serial (like in main code)
    unsigned long wakeTimeout = millis() + 50;
    while (millis() < wakeTimeout && WakeupSerial.available()) {
      uint8_t byte = WakeupSerial.read();

      if (wakeRxPtr == 0 && byte != MMDVM_FRAME_START) {
        continue;
      }

      wakeRxBuffer[wakeRxPtr++] = byte;

      if (wakeRxPtr >= 2) {
        uint8_t frameLength = wakeRxBuffer[1];

        if (wakeRxPtr >= frameLength) {
          if (wakeRxPtr >= 3 && wakeRxBuffer[2] == CMD_GET_VERSION && wakeRxPtr > 4) {
            modemFirmwareVersion = "";
            for (int j = 4; j < wakeRxPtr && wakeRxBuffer[j] != 0x00; j++) {
              if (wakeRxBuffer[j] >= 32 && wakeRxBuffer[j] < 127) {
                modemFirmwareVersion += (char)wakeRxBuffer[j];
              }
            }
            Serial.println();
            Serial.println("[SUCCESS] Got version from wakeup serial: " + modemFirmwareVersion);
            versionFromWakeup = true;
          }
          wakeRxPtr = 0;
        }
      }

      if (wakeRxPtr >= sizeof(wakeRxBuffer)) {
        wakeRxPtr = 0;
      }
    }
  }

  Serial.println();
  Serial.println("[STEP 4] MMDVM wakeup active (SVC LED should be blinking now)");

  // Clear any pending data from main serial
  while (MMDVMSerial.available()) {
    MMDVMSerial.read();
  }

  delay(1000);  // Give modem time to stabilize

  // If we didn't get version from wakeup, try main UART (like in main code)
  if (!versionFromWakeup) {
    Serial.println("[STEP 5] Requesting firmware version on main UART...");
    uint8_t versionCmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};
    MMDVMSerial.write(versionCmd, 3);
    MMDVMSerial.flush();

    // Wait for response on main serial
    Serial.println("[STEP 6] Waiting for response on main serial...");
    unsigned long timeout = millis() + 3000;
    uint8_t rxBuffer[256];
    int rxPtr = 0;
    bool versionReceived = false;

    while (millis() < timeout && !versionReceived) {
      if (MMDVMSerial.available()) {
        uint8_t byte = MMDVMSerial.read();

        if (rxPtr == 0 && byte != MMDVM_FRAME_START) {
          continue;
        }

        rxBuffer[rxPtr++] = byte;

        if (rxPtr >= 2) {
          uint8_t frameLength = rxBuffer[1];

          if (rxPtr >= frameLength) {
            if (rxPtr >= 3 && rxBuffer[2] == CMD_GET_VERSION) {
              modemFirmwareVersion = "";
              for (int i = 4; i < rxPtr && rxBuffer[i] != 0x00; i++) {
                if (rxBuffer[i] >= 32 && rxBuffer[i] < 127) {
                  modemFirmwareVersion += (char)rxBuffer[i];
                }
              }
              versionReceived = true;
            }
            rxPtr = 0;
          }
        }

        if (rxPtr >= sizeof(rxBuffer)) {
          rxPtr = 0;
        }
      }
      delay(10);
    }

    Serial.println();
    if (versionReceived) {
      Serial.println("[SUCCESS] Modem firmware version: " + modemFirmwareVersion);
    } else {
      Serial.println("[FAILED] No response from modem");
      Serial.println("[INFO] This could mean:");
      Serial.println("  - Modem is not powered");
      Serial.println("  - Wrong baud rate");
      Serial.println("  - Incorrect wiring");
      Serial.println("  - Modem is in bootloader mode");
      modemFirmwareVersion = "Unknown";
    }
  }

  // Keep wakeup serial active
  Serial.println();
  Serial.println("[INFO] Keep-alive serial on GPIO 13 will remain active");
  Serial.println("[INFO] This keeps the modem awake - DO NOT STOP IT");
}

// ===== Test Bootloader Connection =====
String testBootloaderConnection() {
  Serial.println("[Test] Testing bootloader...");

  if (!inBootloaderMode) {
    Serial.println("[ERROR] Not in bootloader mode");
    return "ERROR: Not in bootloader mode. Use option 2 first.";
  }

  // Clear RX buffer
  while (MMDVMSerial.available()) {
    MMDVMSerial.read();
  }

  // Send synchronization byte
  MMDVMSerial.write(STM32_SYNC_BYTE);
  MMDVMSerial.flush();

  // Wait for ACK
  unsigned long timeout = millis() + 1000;
  bool ackReceived = false;

  while (millis() < timeout) {
    if (MMDVMSerial.available()) {
      uint8_t response = MMDVMSerial.read();
      Serial.println("[DEBUG] Received: 0x" + String(response, HEX));

      if (response == STM32_ACK) {
        ackReceived = true;
        break;
      } else if (response == STM32_NACK) {
        Serial.println("[ERROR] Received NACK");
        return "ERROR: Bootloader sent NACK";
      }
    }
    delay(10);
  }

  if (ackReceived) {
    Serial.println("[SUCCESS] Bootloader ACK!");
    return "SUCCESS: Bootloader responded with ACK (0x79)";
  } else {
    Serial.println("[FAILED] No ACK");
    return "FAILED: No ACK received from bootloader";
  }
}

// ===== Get Chip ID =====
String getChipID() {
  Serial.println("[ChipID] Getting chip ID...");

  if (!inBootloaderMode) {
    Serial.println("[ERROR] Not in bootloader mode");
    return "ERROR: Not in bootloader mode. Use option 2 first.";
  }

  // Send Get ID command
  if (!sendCommand(STM32_CMD_GID)) {
    return "ERROR: Failed to send Get ID command";
  }

  // Read length byte
  delay(10);
  if (!MMDVMSerial.available()) {
    Serial.println("[ERROR] No length byte");
    return "ERROR: No length byte received";
  }
  uint8_t len = MMDVMSerial.read();

  // Read chip ID (2 bytes)
  delay(10);
  if (MMDVMSerial.available() >= 2) {
    uint16_t chipID = (MMDVMSerial.read() << 8) | MMDVMSerial.read();
    Serial.print("[SUCCESS] Chip ID: 0x");
    Serial.println(chipID, HEX);

    // Wait for final ACK
    waitForAck("ChipID final");

    String result = "Chip ID: 0x" + String(chipID, HEX);

    if (chipID == 0x0410) {
      result += " (STM32F10xxx Medium-density)";
    } else if (chipID == 0x0412) {
      result += " (STM32F10xxx Low-density)";
    } else if (chipID == 0x0414) {
      result += " (STM32F10xxx High-density)";
    }

    return result;
  }

  return "ERROR: Failed to read chip ID";
}

// ===== Get Bootloader Version =====
String getBootloaderVersion() {
  Serial.println("[BootVer] Getting bootloader version...");

  if (!inBootloaderMode) {
    Serial.println("[ERROR] Not in bootloader mode");
    return "ERROR: Not in bootloader mode. Use option 2 first.";
  }

  // Send Get Version command (using STM32_CMD_GET, not STM32_CMD_GV)
  if (!sendCommand(STM32_CMD_GET)) {
    return "ERROR: Failed to send Get command";
  }

  // Read number of bytes
  delay(10);
  if (!MMDVMSerial.available()) {
    Serial.println("[ERROR] No response");
    return "ERROR: No response from bootloader";
  }

  uint8_t numBytes = MMDVMSerial.read();
  Serial.println("[INFO] Bootloader supports " + String(numBytes + 1) + " commands");

  // Read bootloader version
  delay(10);
  if (MMDVMSerial.available()) {
    uint8_t version = MMDVMSerial.read();
    Serial.print("[SUCCESS] Bootloader version: 0x");
    Serial.println(version, HEX);

    // Read supported commands
    String commands = "";
    for (int i = 0; i < numBytes && MMDVMSerial.available(); i++) {
      uint8_t cmd = MMDVMSerial.read();
      commands += "0x" + String(cmd, HEX) + " ";
    }

    // Wait for final ACK
    waitForAck("Get final");

    return "Bootloader v0x" + String(version, HEX) + " | Supports " + String(numBytes + 1) + " commands: " + commands;
  }

  return "ERROR: Failed to read bootloader version";
}

bool writeMemory(uint32_t address, uint8_t* data, uint16_t length) {
  if (!sendCommand(STM32_CMD_WM)) return false;

  // Send address
  uint8_t addrBytes[4] = {
    (uint8_t)(address >> 24),
    (uint8_t)(address >> 16),
    (uint8_t)(address >> 8),
    (uint8_t)address
  };

  uint8_t addrChecksum = addrBytes[0] ^ addrBytes[1] ^ addrBytes[2] ^ addrBytes[3];
  MMDVMSerial.write(addrBytes, 4);
  MMDVMSerial.write(addrChecksum);
  MMDVMSerial.flush();

  if (!waitForAck("Address")) return false;

  // Send data
  MMDVMSerial.write((uint8_t)(length - 1));

  uint8_t dataChecksum = (uint8_t)(length - 1);
  for (uint16_t i = 0; i < length; i++) {
    MMDVMSerial.write(data[i]);
    dataChecksum ^= data[i];
  }
  MMDVMSerial.write(dataChecksum);
  MMDVMSerial.flush();

  return waitForAck("Write");
}
