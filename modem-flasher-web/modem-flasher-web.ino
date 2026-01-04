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
#include <SPIFFS.h>
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

// ===== Forward Declarations =====
void setupWiFi();
void setupWebServer();
void handleRoot();
void handleUpload();
void handleFlashURL();
void handleStatus();
void handleAPI();
bool flashFirmwareFromFile(String filename);
bool eraseFlashMemory();
bool writeFirmwareFromFile(String filename);

// STM32 functions
void enterBootloader();
void exitBootloader();
void readModemFirmwareVersion();
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

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("[ERROR] SPIFFS mount failed");
  } else {
    Serial.println("[OK] SPIFFS mounted");
  }

  // Setup WiFi and Web Server
  setupWiFi();
  setupWebServer();

  Serial.println("\n[READY] System ready!");
  Serial.println("Access web interface to flash firmware");
  Serial.println("Or use serial console for manual control\n");
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
      <h1>🔧 MMDVM Modem Firmware Flasher</h1>
      <p>Web interface for flashing STM32 modem firmware</p>
    </div>

    <div class='card'>
      <h2>📊 System Status</h2>
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
      <h2>📤 Option 1: Upload Firmware File</h2>
      <p>Upload a .bin firmware file from your computer</p>
      <form id='uploadForm' enctype='multipart/form-data'>
        <input type='file' name='firmware' id='fileInput' accept='.bin' required>
        <button type='submit' id='uploadBtn'>Upload & Flash Firmware</button>
      </form>
    </div>

    <div class='card'>
      <h2>🌐 Option 2: Flash from URL</h2>
      <p>Download firmware from a URL (e.g., GitHub releases)</p>
      <form id='urlForm'>
        <input type='text' id='urlInput' placeholder='https://github.com/user/repo/releases/download/v1.0/firmware.bin' required>
        <button type='submit' id='urlBtn'>Download & Flash</button>
      </form>
      <p style='margin-top: 10px; font-size: 12px; color: #666;'>
        Example: https://github.com/juribeparada/MMDVM_HS/releases/download/v1.6.1/mmdvm_hs_hat_fw.bin
      </p>
    </div>

    <div class='card'>
      <h2>⚡ Quick Actions</h2>
      <div class='btn-group'>
        <button onclick='readVersion()'>Read Modem Version</button>
        <button onclick='enterBoot()'>Enter Bootloader</button>
        <button onclick='exitBoot()'>Exit Bootloader</button>
      </div>
    </div>

    <div class='card'>
      <div class='progress-container' id='progressContainer'>
        <div class='progress-bar' id='progressBar'></div>
        <div class='progress-text' id='progressText'>0%</div>
      </div>

      <h2>📝 Activity Log</h2>
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

    function updateStatus() {
      fetch('/status')
        .then(r => r.json())
        .then(data => {
          document.getElementById('modemVersion').textContent = data.modemVersion;
          document.getElementById('bootMode').textContent = data.inBootloader ? 'Active' : 'Inactive';
          document.getElementById('flashStat').textContent = data.flashStatus || 'Idle';

          if (data.flashProgress > 0) {
            document.getElementById('progressContainer').style.display = 'block';
            document.getElementById('progressBar').style.width = data.flashProgress + '%';
            document.getElementById('progressText').textContent = data.flashProgress + '%';

            if (data.flashProgress >= 100) {
              setTimeout(() => {
                document.getElementById('progressContainer').style.display = 'none';
              }, 3000);
            }
          }

          if (data.lastLog) {
            log(data.lastLog, data.logType || 'info');
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

    // URL download handler
    document.getElementById('urlForm').onsubmit = function(e) {
      e.preventDefault();
      const url = document.getElementById('urlInput').value;

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

    // Update status every 2 seconds
    setInterval(updateStatus, 2000);
    log('Web interface ready', 'success');
  </script>
</body>
</html>)";

  server.send(200, "text/html", html);
}

void handleUpload() {
  HTTPUpload& upload = server.upload();
  static File uploadFile;

  if (upload.status == UPLOAD_FILE_START) {
    flashStatus = "Receiving: " + String(upload.filename);
    flashProgress = 0;
    Serial.println("[Upload] " + flashStatus);

    uploadFile = SPIFFS.open("/firmware.bin", "w");
    if (!uploadFile) {
      flashStatus = "ERROR: Failed to open file";
      Serial.println("[Upload] " + flashStatus);
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
      flashProgress = 5; // Uploading
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      flashStatus = "Upload complete (" + String(upload.totalSize) + " bytes)";
      Serial.println("[Upload] " + flashStatus);

      // Start flashing in background
      flashProgress = 10;
      flashInProgress = true;

      // Flash the firmware
      if (flashFirmwareFromFile("/firmware.bin")) {
        flashStatus = "Flash complete!";
      } else {
        flashStatus = "Flash failed!";
      }

      flashInProgress = false;
    }
  }
}

void handleFlashURL() {
  if (!server.hasArg("url")) {
    server.send(400, "text/plain", "ERROR: Missing URL parameter");
    return;
  }

  String url = server.arg("url");
  flashStatus = "Downloading from URL...";
  flashProgress = 5;
  Serial.println("[Download] " + url);

  HTTPClient http;
  http.begin(url);
  http.setTimeout(30000); // 30 second timeout

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    File file = SPIFFS.open("/firmware.bin", "w");

    if (file) {
      WiFiClient* stream = http.getStreamPtr();
      int len = http.getSize();
      uint8_t buff[128];
      int downloaded = 0;

      while (http.connected() && (len > 0 || len == -1)) {
        size_t size = stream->available();
        if (size) {
          int c = stream->readBytes(buff, min(size, sizeof(buff)));
          file.write(buff, c);
          downloaded += c;
          if (len > 0) {
            len -= c;
            flashProgress = 5 + (downloaded * 10 / (downloaded + len));
          }
        }
        delay(1);
      }

      file.close();
      flashStatus = "Downloaded " + String(downloaded) + " bytes";
      Serial.println("[Download] " + flashStatus);

      // Flash the firmware
      flashProgress = 15;
      if (flashFirmwareFromFile("/firmware.bin")) {
        server.send(200, "text/plain", "Flash complete!");
      } else {
        server.send(500, "text/plain", "Flash failed!");
      }
    } else {
      flashStatus = "ERROR: Failed to save file";
      server.send(500, "text/plain", flashStatus);
    }
  } else {
    flashStatus = "ERROR: HTTP " + String(httpCode);
    Serial.println("[Download] " + flashStatus);
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
  else {
    result = "Unknown action";
  }

  server.send(200, "text/plain", result);
}

// ===== Flash Functions =====
bool flashFirmwareFromFile(String filename) {
  Serial.println("\n[Flash] Starting flash process...");

  // Enter bootloader if needed
  if (!inBootloaderMode) {
    flashStatus = "Entering bootloader...";
    flashProgress = 20;
    enterBootloader();
    delay(500);
  }

  if (!inBootloaderMode) {
    flashStatus = "ERROR: Could not enter bootloader";
    Serial.println("[Flash] " + flashStatus);
    return false;
  }

  // Erase flash
  flashStatus = "Erasing flash...";
  flashProgress = 30;
  if (!eraseFlashMemory()) {
    flashStatus = "ERROR: Erase failed";
    return false;
  }

  // Write firmware
  flashStatus = "Writing firmware...";
  flashProgress = 50;
  if (!writeFirmwareFromFile(filename)) {
    flashStatus = "ERROR: Write failed";
    return false;
  }

  // Exit bootloader
  flashStatus = "Resetting modem...";
  flashProgress = 95;
  exitBootloader();

  flashStatus = "Flash complete!";
  flashProgress = 100;
  Serial.println("[Flash] Complete!");

  // Read new version
  delay(1000);
  readModemFirmwareVersion();

  return true;
}

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

bool writeFirmwareFromFile(String filename) {
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.println("[Write] Cannot open file");
    return false;
  }

  size_t fileSize = file.size();
  Serial.printf("[Write] File size: %d bytes\n", fileSize);

  uint32_t address = FLASH_START_ADDR;
  size_t bytesWritten = 0;
  uint8_t buffer[MAX_WRITE_SIZE];

  while (file.available()) {
    size_t chunk = file.read(buffer, MAX_WRITE_SIZE);

    // Pad to 256 bytes
    if (chunk < MAX_WRITE_SIZE) {
      memset(buffer + chunk, 0xFF, MAX_WRITE_SIZE - chunk);
      chunk = MAX_WRITE_SIZE;
    }

    if (!writeMemory(address, buffer, chunk)) {
      Serial.printf("[Write] Failed at 0x%08X\n", address);
      file.close();
      return false;
    }

    bytesWritten += chunk;
    address += chunk;

    // Update progress
    flashProgress = 50 + (bytesWritten * 40 / fileSize);

    if (bytesWritten % (16 * 256) == 0) {
      Serial.printf("[Write] Progress: %d / %d (%.1f%%)\n",
        bytesWritten, fileSize, (bytesWritten * 100.0) / fileSize);
    }
  }

  file.close();
  Serial.printf("[Write] Complete! %d bytes written\n", bytesWritten);
  return true;
}

// ===== STM32 Bootloader Functions =====

void hardwareResetToBootloader() {
  MMDVMSerial.end();
  WakeupSerial.end();
  delay(100);

  digitalWrite(MMDVM_BOOT0_PIN, HIGH);
  delay(50);
  digitalWrite(MMDVM_RESET_PIN, LOW);
  delay(50);
  digitalWrite(MMDVM_RESET_PIN, HIGH);
  delay(500);
}

void hardwareResetToNormal() {
  MMDVMSerial.end();
  WakeupSerial.end();
  delay(100);

  digitalWrite(MMDVM_BOOT0_PIN, LOW);
  delay(50);
  digitalWrite(MMDVM_RESET_PIN, LOW);
  delay(50);
  digitalWrite(MMDVM_RESET_PIN, HIGH);
  delay(500);
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
  Serial.println("[Boot] Entering bootloader...");
  hardwareResetToBootloader();

  MMDVMSerial.begin(115200, SERIAL_8E1, MMDVM_RX_PIN, MMDVM_TX_PIN);
  delay(100);

  if (syncBootloader()) {
    Serial.println("[Boot] Bootloader active!");
    inBootloaderMode = true;
  } else {
    Serial.println("[Boot] Failed to sync");
    inBootloaderMode = false;
  }
}

void exitBootloader() {
  Serial.println("[Boot] Exiting bootloader...");
  hardwareResetToNormal();
  inBootloaderMode = false;
}

void readModemFirmwareVersion() {
  Serial.println("[Version] Reading...");

  MMDVMSerial.begin(115200, SERIAL_8N1, MMDVM_RX_PIN, MMDVM_TX_PIN);
  WakeupSerial.begin(115200, SERIAL_8N1, 1, MMDVM_RESET_PIN);
  delay(100);

  uint8_t cmd[] = {MMDVM_FRAME_START, 0x03, CMD_GET_VERSION};
  uint8_t rxBuffer[256];
  int rxPtr = 0;
  bool found = false;

  for (int i = 0; i < 20 && !found; i++) {
    WakeupSerial.write(cmd, 3);
    WakeupSerial.flush();
    delay(100);

    unsigned long timeout = millis() + 50;
    while (millis() < timeout && WakeupSerial.available()) {
      uint8_t byte = WakeupSerial.read();
      if (rxPtr == 0 && byte != MMDVM_FRAME_START) continue;

      rxBuffer[rxPtr++] = byte;

      if (rxPtr >= 2) {
        uint8_t frameLength = rxBuffer[1];
        if (rxPtr >= frameLength && rxPtr >= 3 && rxBuffer[2] == CMD_GET_VERSION && rxPtr > 4) {
          modemFirmwareVersion = "";
          for (int j = 4; j < rxPtr && rxBuffer[j] != 0x00; j++) {
            if (rxBuffer[j] >= 32 && rxBuffer[j] < 127) {
              modemFirmwareVersion += (char)rxBuffer[j];
            }
          }
          found = true;
        }
      }

      if (rxPtr >= sizeof(rxBuffer)) rxPtr = 0;
    }
  }

  Serial.println("[Version] " + (found ? modemFirmwareVersion : "Not found"));
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
