/*
 * ESP32 MMDVM Hotspot 
 *
 * This code provides a basic framework for using an ESP32 with an MMDVM hat
 * to create a DMR hotspot similar to Pi-Star.
 *
 * Hardware Requirements:
 * - ESP32 Development Board
 * - MMDVM Hat (compatible with ESP32 3.3V logic or use level shifters)
 * - Antenna and appropriate radio frontend
 *
 * Connections:
 * - MMDVM RX -> ESP32 GPIO16 (RX2)
 * - MMDVM TX -> ESP32 GPIO17 (TX2)
 * - MMDVM PTT -> ESP32 GPIO4
 * - MMDVM COSLED -> ESP32 GPIO2
 *
 * Libraries needed:
 * - WiFi (built-in)
 * - WiFiUDP (built-in)
 * - WebServer (built-in)
 * - ESPmDNS (built-in)
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "mbedtls/md.h"
#include "nvs_flash.h"
#include <Update.h>
#include <HTTPClient.h>
#include "config.h"
#include "webpages.h"

#if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(3,0,0)
#include <ETHClass2.h>       //Is to use the modified ETHClass
#define ETH  ETH2
#else
#include <ETH.h>
#endif
static bool eth_connected = false;
#include <SPI.h>
#include <SD.h>

// ESP32-S3 USB Serial configuration
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
#if ARDUINO_USB_MODE
#warning "USB MODE is enabled - Serial will work via USB CDC"
#endif
#if ARDUINO_USB_CDC_ON_BOOT
#warning "USB CDC ON BOOT is enabled"
#endif
#endif

// ===== Configuration from config.h =====
// WiFi Settings
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Fallback AP mode settings (from config.h)
const char* ap_ssid = AP_SSID;
const char* ap_password = AP_PASSWORD;

// DMR Network Settings (can be overridden by stored config)
String dmr_callsign = DMR_CALLSIGN;
uint32_t dmr_id = DMR_ID;
String dmr_server = DMR_SERVER;
String dmr_password = DMR_PASSWORD;
uint8_t dmr_essid = 0;  // ESSID 0-99
const int dmr_port = DMR_PORT;

// Additional DMR Configuration
uint32_t dmr_rx_freq = 434000000;  // RX Frequency in Hz
uint32_t dmr_tx_freq = 434000000;  // TX Frequency in Hz
uint8_t dmr_power = 10;            // Power 0-99
uint8_t dmr_color_code = 1;        // Color Code 0-15
float dmr_latitude = 0.0;          // Latitude
float dmr_longitude = 0.0;         // Longitude
int dmr_height = 0;                // Height in meters
String dmr_location = DMR_LOCATION;
String dmr_description = DMR_DESCRIPTION;
String dmr_url = DMR_URL;

// Hostname setting
String device_hostname = MDNS_HOSTNAME;

// Verbose logging setting (shows keepalive messages)
bool verbose_logging = false;

// Web interface credentials (from config.h)
String web_username = WEB_USERNAME;
String web_password = WEB_PASSWORD;

// MMDVM Settings
#define SERIAL_BAUD MMDVM_SERIAL_BAUD
#define MMDVM_SERIAL Serial2
#define PTT_PIN MMDVM_PTT_PIN
#define COS_LED_PIN MMDVM_COS_LED_PIN
#define RX_PIN MMDVM_RX_PIN
#define TX_PIN MMDVM_TX_PIN

// ===== Protocol Constants =====
#define MMDVM_FRAME_START 0xE0

// MMDVM Commands
#define CMD_GET_VERSION 0x00
#define CMD_GET_STATUS 0x01
#define CMD_SET_CONFIG 0x02
#define CMD_SET_MODE 0x03
#define CMD_SET_FREQ 0x04
#define CMD_CAL_DATA 0x08
#define CMD_DMR_DATA1 0x18
#define CMD_DMR_DATA2 0x19
#define CMD_DMR_LOST 0x1A
#define CMD_DMR_SHORTLC 0x1B
#define CMD_DMR_START 0x1C
#define CMD_DMR_ABORT 0x1D

// DMR Protocol
#define DMR_SLOT1 0x00
#define DMR_SLOT2 0x01

// ===== Global Variables =====
WiFiUDP udp;
WebServer server(80);
Preferences preferences;
bool wifiConnected = false;
bool apMode = false;
bool mmdvmReady = false;
bool dmrLoggedIn = false;
uint32_t currentTalkgroup = 0;
String dmrLoginStatus = "Not Connected";
uint8_t rxBuffer[512];
uint8_t txBuffer[512];
int rxBufferPtr = 0;

// DMR Network State Machine
enum class DMR_STATE {
  DISCONNECTED,
  WAITING_LOGIN,
  WAITING_AUTH,
  WAITING_CONFIG,
  CONNECTED
};
DMR_STATE dmrState = DMR_STATE::DISCONNECTED;
uint8_t dmrSalt[4];

// Serial Monitor Buffer (SERIAL_LOG_SIZE defined in webpages.h)
String serialLog[SERIAL_LOG_SIZE];
int serialLogIndex = 0;

unsigned long lastKeepalive = 0;
const unsigned long KEEPALIVE_INTERVAL = 5000;  // 5 seconds

// Store alternate WiFi credentials
// Alternate WiFi Networks (up to 5) - labels from config.h
WiFiNetwork wifiNetworks[5] = {
  { WIFI_SLOT1_LABEL, "", "" },
  { WIFI_SLOT2_LABEL, "", "" },
  { WIFI_SLOT3_LABEL, "", "" },
  { WIFI_SLOT4_LABEL, "", "" },
  { WIFI_SLOT5_LABEL, "", "" }
};

// Firmware version from config.h
String firmwareVersion = FIRMWARE_VERSION;

// LED Status Control (STATUS_LED_PIN defined in config.h)

enum class LED_MODE {
  OFF,
  STEADY,      // Connected to WiFi
  FAST_BLINK,  // Connecting to WiFi
  SLOW_BLINK   // Access Point mode
};
LED_MODE currentLedMode = LED_MODE::OFF;
unsigned long lastLedToggle = 0;
bool ledState = false;

// Mode Enable/Disable Settings (defaults from config.h)
bool mode_dmr_enabled = DEFAULT_MODE_DMR;
bool mode_dstar_enabled = DEFAULT_MODE_DSTAR;
bool mode_ysf_enabled = DEFAULT_MODE_YSF;
bool mode_p25_enabled = DEFAULT_MODE_P25;
bool mode_nxdn_enabled = DEFAULT_MODE_NXDN;
bool mode_pocsag_enabled = DEFAULT_MODE_POCSAG;

// ===== Function Prototypes =====
void setupWiFi();
void setupAccessPoint();
void setupEthernet();
void WiFiEvent(arduino_event_id_t event);
void setupWebServer();
void setupMMDVM();
void loadConfig();
void saveConfig();
void handleMMDVMSerial();
void handleNetwork();
void sendMMDVMCommand(uint8_t cmd, uint8_t* data, uint16_t length);
void processMMDVMFrame();
void updateStatusLED();
void setLEDMode(LED_MODE mode);
void sendDMRKeepalive();
void connectToDMRNetwork();
void sendDMRAuth();
void sendDMRConfig();
void logSerial(String message);
void logSerialVerbose(String message);
// Web handlers are defined in webpages.h

void setup() {
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
  // ESP32-S3 USB CDC (native USB support)
  Serial.begin(115200);
  delay(1000);
  // Wait for USB Serial to be ready (ESP32-S3 native USB)
  while (!Serial && millis() < 3000) {
    delay(10);
  }
#else
  Serial.begin(115200);
  delay(1000);
#endif

  logSerial("\n\n=== ESP32 MMDVM Hotspot ===");
  logSerial("Initializing...");

  // Load saved configuration
  loadConfig();

  // Setup GPIO
  pinMode(PTT_PIN, OUTPUT);
  pinMode(COS_LED_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);  // Setup status LED
  digitalWrite(PTT_PIN, LOW);
  digitalWrite(COS_LED_PIN, LOW);
  digitalWrite(STATUS_LED_PIN, LOW);

  // Setup MMDVM Serial
  MMDVM_SERIAL.begin(SERIAL_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  logSerial("MMDVM Serial initialized");

  // Setup Network (Ethernet or WiFi)
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
  setupEthernet();
#else
  setupWiFi();
#endif

// Setup Web Server
//setupWebServer();
#if ENABLE_WEBSERVER
  setupWebServer();
#endif

// Start mDNS
// if (MDNS.begin(device_hostname.c_str())) {
//   logSerial("mDNS started: http://" + device_hostname + ".local");
// }
#if ENABLE_MDNS
  if (MDNS.begin(device_hostname.c_str())) {
    logSerial("mDNS started: http://" + device_hostname + ".local");
  }
#endif

  // Initialize MMDVM
  setupMMDVM();

  // Connect to Networks (based on enabled modes)
  if (wifiConnected) {
    // DMR Network Connection
    if (mode_dmr_enabled) {
      connectToDMRNetwork();
    } else {
      logSerial("DMR mode is disabled - skipping DMR network connection");
    }

    // D-Star Network Connection (not implemented yet)
    if (mode_dstar_enabled) {
      logSerial("D-Star mode is enabled but network connection not implemented yet");
    } else {
      logSerial("D-Star mode is disabled by default (not implemented yet)");
    }

    // YSF Network Connection (not implemented yet)
    if (mode_ysf_enabled) {
      logSerial("YSF mode is enabled but network connection not implemented yet");
    } else {
      logSerial("YSF mode is disabled by default (not implemented yet)");
    }

    // P25 Network Connection (not implemented yet)
    if (mode_p25_enabled) {
      logSerial("P25 mode is enabled but network connection not implemented yet");
    } else {
      logSerial("P25 mode is disabled by default (not implemented yet)");
    }

    // NXDN Network Connection (not implemented yet)
    if (mode_nxdn_enabled) {
      logSerial("NXDN mode is enabled but network connection not implemented yet");
    } else {
      logSerial("NXDN mode is disabled by default (not implemented yet)");
    }

    // POCSAG Network Connection (not implemented yet)
    if (mode_pocsag_enabled) {
      logSerial("POCSAG mode is enabled but network connection not implemented yet");
    } else {
      logSerial("POCSAG mode is disabled by default (not implemented yet)");
    }
  } else {
    logSerial("WiFi not connected - skipping all network connections");
  }

  logSerial("Setup complete!");
  if (apMode) {
    logSerial("Access Point Mode - Connect to: " + String(ap_ssid));
    logSerial("Web Interface: http://192.168.4.1");
  } else if (wifiConnected) {
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
    if (eth_connected) {
      logSerial("Web Interface: http://" + ETH.localIP().toString());
    }
#else
    logSerial("Web Interface: http://" + WiFi.localIP().toString());
#endif
  }
}

void loop() {
  // Update status LED
  updateStatusLED();

  // Handle web server
  server.handleClient();

  // Handle MMDVM serial communication
  handleMMDVMSerial();

  // Handle network communication
  if (wifiConnected) {
    handleNetwork();

    // Send keepalive packets only if DMR mode is enabled and connected
    if (mode_dmr_enabled && dmrLoggedIn) {
      unsigned long currentMillis = millis();
      if (currentMillis - lastKeepalive >= KEEPALIVE_INTERVAL) {
        sendDMRKeepalive();
        lastKeepalive = currentMillis;
      }
    }
  }

  // Small delay to prevent watchdog issues
  delay(1);
}

void setupWiFi() {
  logSerial("Connecting to WiFi: " + String(ssid));

  setLEDMode(LED_MODE::FAST_BLINK);  // Fast blink while connecting

  // Prevent ESP32 WiFi library from auto-connecting with stored credentials
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);        // Disable WiFi credential storage in flash (must be before disconnect/begin)
  WiFi.setAutoReconnect(false);  // Disable auto-reconnect to prevent using old credentials
  WiFi.disconnect(true, true);   // Disconnect and erase any stored credentials from WiFi library
  delay(100);

  WiFi.setHostname(device_hostname.c_str());                        // Set WiFi hostname (must be before WiFi.begin)
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);  // Clear any previous config
  WiFi.setHostname(device_hostname.c_str());                        // Set hostname again after config
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    updateStatusLED();  // Update LED during connection attempts
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    setLEDMode(LED_MODE::STEADY);  // Steady on when connected
    logSerial("\nWiFi Connected!");
    logSerial("IP Address: " + WiFi.localIP().toString());

    // Start UDP
    udp.begin(LOCAL_PORT);
  } else {
    logSerial("\nWiFi Connection Failed!");

    // Try all configured alternate WiFi networks
    for (int i = 0; i < 5; i++) {
      if (wifiNetworks[i].ssid.length() > 0) {
        logSerial("Trying WiFi [" + wifiNetworks[i].label + "]: " + wifiNetworks[i].ssid);
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);  // Clear any previous config
        WiFi.setHostname(device_hostname.c_str());                        // Set WiFi hostname
        WiFi.begin(wifiNetworks[i].ssid.c_str(), wifiNetworks[i].password.c_str());

        attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 30) {
          updateStatusLED();  // Update LED during connection attempts
          delay(500);
          Serial.print(".");
          attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
          wifiConnected = true;
          setLEDMode(LED_MODE::STEADY);  // Steady on when connected
          logSerial("\nWiFi Connected [" + wifiNetworks[i].label + "]!");
          logSerial("IP Address: " + WiFi.localIP().toString());
          udp.begin(LOCAL_PORT);
          return;
        }
      }
    }

    // If all fails, start Access Point
    logSerial("Starting Access Point mode...");
    setupAccessPoint();
  }
}

void setupAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  setLEDMode(LED_MODE::SLOW_BLINK);  // Slow blink in AP mode

  IPAddress IP = WiFi.softAPIP();
  logSerial("AP IP address: " + IP.toString());
  logSerial("AP SSID: " + String(ap_ssid));
  logSerial("AP Password: " + String(ap_password));

  apMode = true;
}

void WiFiEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      logSerial("ETH Started");
      ETH.setHostname(device_hostname.c_str());
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      logSerial("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      logSerial("ETH MAC: " + ETH.macAddress());
      logSerial("ETH IPv4: " + ETH.localIP().toString());
      if (ETH.fullDuplex()) {
        logSerial("ETH Mode: FULL_DUPLEX");
      }
      logSerial("ETH Speed: " + String(ETH.linkSpeed()) + "Mbps");
      logSerial("ETH Gateway: " + ETH.gatewayIP().toString());
      eth_connected = true;
      wifiConnected = true;
      setLEDMode(LED_MODE::STEADY);

      // Start UDP for DMR network
      udp.begin(LOCAL_PORT);
      logSerial("UDP started on port " + String(LOCAL_PORT));
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      logSerial("ETH Disconnected");
      eth_connected = false;
      wifiConnected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      logSerial("ETH Stopped");
      eth_connected = false;
      wifiConnected = false;
      break;
    default:
      break;
  }
}

void setupEthernet() {
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
  logSerial("Initializing Ethernet (LILYGO T-ETH-ELITE)...");

  WiFi.onEvent(WiFiEvent);

  setLEDMode(LED_MODE::FAST_BLINK);

#ifdef ETH_POWER_PIN
  pinMode(ETH_POWER_PIN, OUTPUT);
  digitalWrite(ETH_POWER_PIN, HIGH);
#endif

  // ESP32-S3 uses W5500 Ethernet chip
  if (!ETH.begin(ETH_PHY_W5500, 1, ETH_CS_PIN, ETH_INT_PIN, ETH_RST_PIN,
                 SPI3_HOST,
                 ETH_SCLK_PIN, ETH_MISO_PIN, ETH_MOSI_PIN)) {
    logSerial("ETH start Failed!");
    logSerial("ERROR: Could not initialize Ethernet hardware!");
    logSerial("Please check your wiring and board configuration.");
  } else {
    logSerial("ETH initialization started");
    logSerial("Ethernet will connect in background...");
    // Don't wait - let it connect in background via event handler
    // UDP will be started when ETH_GOT_IP event fires
  }
#endif
}

void setupMMDVM() {
  logSerial("Initializing MMDVM...");
  delay(1000);

  // Request version
  uint8_t cmd = CMD_GET_VERSION;
  sendMMDVMCommand(CMD_GET_VERSION, NULL, 0);
  delay(100);

  // Set configuration
  uint8_t config[20];
  memset(config, 0, sizeof(config));

  // Basic config - adjust based on your MMDVM hat
  config[0] = (MMDVM_RX_INVERT ? 0x01 : 0x00);
  config[1] = (MMDVM_TX_INVERT ? 0x01 : 0x00);
  config[2] = (MMDVM_PTT_INVERT ? 0x01 : 0x00);
  config[3] = MMDVM_TX_DELAY;
  config[4] = 0x00;  // Mode Hang
  config[5] = MMDVM_RX_LEVEL;
  config[6] = MMDVM_TX_LEVEL;

  sendMMDVMCommand(CMD_SET_CONFIG, config, 7);
  delay(100);

  // Set mode to DMR
  uint8_t mode = 0x02;  // DMR mode
  sendMMDVMCommand(CMD_SET_MODE, &mode, 1);
  delay(100);

  logSerial("MMDVM Initialized");
  mmdvmReady = true;
}

void sendMMDVMCommand(uint8_t cmd, uint8_t* data, uint16_t length) {
  uint8_t buffer[512];
  buffer[0] = MMDVM_FRAME_START;
  buffer[1] = length + 3;  // Length includes START, LEN, and CMD
  buffer[2] = cmd;

  if (data != NULL && length > 0) {
    memcpy(&buffer[3], data, length);
  }

  MMDVM_SERIAL.write(buffer, length + 3);
}

void handleMMDVMSerial() {
  while (MMDVM_SERIAL.available()) {
    uint8_t byte = MMDVM_SERIAL.read();

    if (rxBufferPtr == 0 && byte != MMDVM_FRAME_START) {
      continue;  // Wait for frame start
    }

    rxBuffer[rxBufferPtr++] = byte;

    // Check if we have enough bytes to read length
    if (rxBufferPtr >= 2) {
      uint8_t frameLength = rxBuffer[1];

      // Check if we have a complete frame
      if (rxBufferPtr >= frameLength) {
        processMMDVMFrame();
        rxBufferPtr = 0;
      }
    }

    // Prevent buffer overflow
    if (rxBufferPtr >= sizeof(rxBuffer)) {
      rxBufferPtr = 0;
    }
  }
}

void processMMDVMFrame() {
  if (rxBufferPtr < 3) return;

  uint8_t cmd = rxBuffer[2];

  switch (cmd) {
    case CMD_GET_VERSION:
      {
        String version = "MMDVM Version: ";
        for (int i = 3; i < rxBufferPtr; i++) {
          version += (char)rxBuffer[i];
        }
        logSerial(version);
      }
      break;

    case CMD_GET_STATUS:
      {
        String status = "MMDVM Status - Mode: " + String(rxBuffer[3]) + " TX: " + String(rxBuffer[4]) + " Overflow: " + String(rxBuffer[7]);
        logSerial(status);
      }
      break;

    case CMD_DMR_DATA1:
    case CMD_DMR_DATA2:
      // DMR data received from MMDVM - forward to network
      if (wifiConnected) {
        // Extract DMR frame and send to network
        uint16_t dataLen = rxBufferPtr - 3;
        udp.beginPacket(dmr_server.c_str(), dmr_port);
        udp.write(&rxBuffer[3], dataLen);
        udp.endPacket();

        logSerial("DMR data forwarded to network");
        digitalWrite(COS_LED_PIN, HIGH);
        delay(50);
        digitalWrite(COS_LED_PIN, LOW);
      }
      break;

    default:
      logSerial("Unknown MMDVM command: 0x" + String(cmd, HEX));
      break;
  }
}

void handleNetwork() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    uint8_t packet[512];
    int len = udp.read(packet, sizeof(packet));

    if (len > 0) {
      // Check if this is a keepalive packet first (for conditional logging)
      bool isKeepalive = (memcmp(packet, "MSTPONG", 7) == 0 && len >= 7);

      // Debug: Log packet details
      String hexDump = "RX [" + String(len) + "]: ";
      for (int i = 0; i < min(len, 16); i++) {
        if (packet[i] < 0x10) hexDump += "0";
        hexDump += String(packet[i], HEX);
        hexDump += " ";
      }

      // Use verbose logging for keepalive packets (always USB, conditionally web)
      if (isKeepalive) {
        logSerialVerbose(hexDump);
      } else {
        logSerial(hexDump);
      }

      // Check for BrandMeister responses (binary comparison)
      if (len >= 4) {
        // Check for negative acknowledgment (MSTNAK)
        if (memcmp(packet, "MSTNAK", 6) == 0 && len >= 6) {
          dmrLoggedIn = false;
          dmrLoginStatus = "Login Failed";

          // Log which stage failed
          String stageMsg = "BrandMeister NAK at stage: ";
          switch (dmrState) {
            case DMR_STATE::WAITING_LOGIN: stageMsg += "LOGIN"; break;
            case DMR_STATE::WAITING_AUTH: stageMsg += "AUTH"; break;
            case DMR_STATE::WAITING_CONFIG: stageMsg += "CONFIG"; break;
            default: stageMsg += "UNKNOWN";
          }
          logSerial(stageMsg);
          logSerial("STOPPING - Please check configuration and reboot");

          // Stop trying to prevent ban
          dmrState = DMR_STATE::DISCONNECTED;
        }
        // Check for login acknowledgment (RPTACK)
        else if (memcmp(packet, "RPTACK", 6) == 0 && len >= 10) {
          // RPTACK response includes salt for password authentication
          memcpy(dmrSalt, packet + 6, 4);

          // Debug: Show salt
          String saltHex = "Salt: ";
          for (int i = 0; i < 4; i++) {
            if (dmrSalt[i] < 0x10) saltHex += "0";
            saltHex += String(dmrSalt[i], HEX);
          }
          logSerial(saltHex);

          switch (dmrState) {
            case DMR_STATE::WAITING_LOGIN:
              logSerial("Login ACK received (state: WAITING_LOGIN), sending auth...");
              dmrState = DMR_STATE::WAITING_AUTH;
              sendDMRAuth();
              break;
            case DMR_STATE::WAITING_AUTH:
              logSerial("Auth ACK received (state: WAITING_AUTH), sending config...");
              dmrState = DMR_STATE::WAITING_CONFIG;
              sendDMRConfig();
              break;
            case DMR_STATE::WAITING_CONFIG:
              logSerial("Config ACK - CONNECTED!");
              dmrLoggedIn = true;
              dmrLoginStatus = "Connected";
              dmrState = DMR_STATE::CONNECTED;
              logSerial("DMR Network fully connected and operational!");
              break;
            case DMR_STATE::DISCONNECTED:
              // Don't retry after NAK to prevent bans
              logSerial("RPTACK received but in DISCONNECTED state - ignoring");
              break;
            default:
              logSerial("RPTACK received in unexpected state: " + String((int)dmrState));
              break;
          }
        }
        // Check for ping response (MSTPONG)
        else if (memcmp(packet, "MSTPONG", 7) == 0 && len >= 7) {
          logSerialVerbose("Keepalive ACK");
        }
        // DMR data packet
        else if (memcmp(packet, "DMRD", 4) == 0 && len > 15) {
          logSerial("DMR data packet received");

          // Parse and forward to MMDVM
          if (mmdvmReady) {
            uint8_t cmd = CMD_DMR_DATA1;  // or CMD_DMR_DATA2 based on slot
            sendMMDVMCommand(cmd, packet, len);
          }
        }
      }
    }
  }
}

void connectToDMRNetwork() {
  dmrLoginStatus = "Connecting...";
  dmrLoggedIn = false;
  dmrState = DMR_STATE::WAITING_LOGIN;

  logSerial("Connecting to DMR Network...");
  logSerial("Server: " + dmr_server + ":" + String(dmr_port));
  logSerial("Callsign: " + dmr_callsign + " ID: " + String(dmr_id));
  if (dmr_essid > 0) {
    logSerial("ESSID: " + String(dmr_essid));
  }

  // Create proper BrandMeister login packet
  // Format: "RPTL" (4 bytes) + DMR_ID (4 bytes binary, big-endian)
  uint8_t loginPacket[8];
  uint32_t id_to_send = dmr_id;

  // If ESSID is set, append it to the ID (e.g., 204115201 for ID 2041152 with ESSID 01)
  if (dmr_essid > 0) {
    id_to_send = dmr_id * 100 + dmr_essid;
  }

  // Packet format: "RPTL" + 4-byte binary DMR ID (big-endian)
  memcpy(loginPacket, "RPTL", 4);
  loginPacket[4] = (id_to_send >> 24) & 0xFF;  // Most significant byte
  loginPacket[5] = (id_to_send >> 16) & 0xFF;
  loginPacket[6] = (id_to_send >> 8) & 0xFF;
  loginPacket[7] = id_to_send & 0xFF;  // Least significant byte

  udp.beginPacket(dmr_server.c_str(), dmr_port);
  udp.write(loginPacket, 8);
  udp.endPacket();

  logSerial("Login packet sent, ID: " + String(id_to_send));
}

void sendDMRAuth() {
// Send RPTK (authorization) packet with SHA256(salt + password)
// Format: "RPTK" + DMR_ID (4 bytes binary) + SHA256 hash (32 bytes binary)
// SHA256 input: salt (4 binary bytes) + password (ASCII string)

// Debug: Show password being used (with more detail)
#if DEBUG_PASSWORD
  String passDebug = "Using password: length=" + String(dmr_password.length());
  if (dmr_password.length() > 0) {
    passDebug += ", last4=" + dmr_password.substring(max(0, (int)dmr_password.length() - 4));
  } else {
    passDebug += " [EMPTY!]";
  }
  logSerial(passDebug);
#endif

  // Calculate SHA256 hash of (salt + password)
  size_t passLen = dmr_password.length();
  size_t inputLen = 4 + passLen;
  uint8_t* input = new uint8_t[inputLen];

  // Salt is 4 binary bytes, not hex string
  memcpy(input, dmrSalt, 4);
  memcpy(input + 4, dmr_password.c_str(), passLen);

  // Calculate SHA256 hash (binary output, not hex)
  uint8_t hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, input, inputLen);
  mbedtls_md_finish(&ctx, hash);
  mbedtls_md_free(&ctx);

  delete[] input;

  // Build RPTK packet: "RPTK" (4 bytes) + ID (4 bytes binary) + hash (32 bytes binary)
  uint8_t authPacket[40];
  uint32_t id_to_send = dmr_id;
  if (dmr_essid > 0) {
    id_to_send = dmr_id * 100 + dmr_essid;
  }

  memcpy(authPacket, "RPTK", 4);
  authPacket[4] = (id_to_send >> 24) & 0xFF;  // ID as 4 binary bytes (big-endian)
  authPacket[5] = (id_to_send >> 16) & 0xFF;
  authPacket[6] = (id_to_send >> 8) & 0xFF;
  authPacket[7] = id_to_send & 0xFF;
  memcpy(authPacket + 8, hash, 32);  // SHA256 hash as 32 binary bytes

  udp.beginPacket(dmr_server.c_str(), dmr_port);
  udp.write(authPacket, 40);
  udp.endPacket();

  logSerial("Auth packet sent (40 bytes)");
}

void sendDMRConfig() {
  // Send RPTC (configuration) packet
  // Format: "RPTC" (4 bytes) + DMR_ID (4 bytes binary) + config string (294 bytes) = 302 bytes
  // Using larger buffer to prevent overflow during snprintf, then copy exact 294 bytes
  char configString[400];
  memset(configString, 0, sizeof(configString));

  uint32_t id_to_send = dmr_id;
  if (dmr_essid > 0) {
    id_to_send = dmr_id * 100 + dmr_essid;
  }

  // Config format (callsign + basic info) - 294 bytes
  // Format per MMDVMHost writeConfig():
  // %-8.8s%09u%09u%02u%02u%8.8s%9.9s%03d%-20.20s%-19.19s%c%-124.124s%-40.40s%-40.40s

  // Format latitude and longitude properly as floats with exact widths
  char latitude[20];
  char longitude[20];
  sprintf(latitude, "%08f", dmr_latitude);    // %08f = 8 chars total
  sprintf(longitude, "%09f", dmr_longitude);  // %09f = 9 chars total

  // Validate and limit values
  unsigned int power = dmr_power;
  if (power > 99) power = 99;
  int height = dmr_height;
  if (height > 999) height = 999;

  snprintf(configString, sizeof(configString),
           "%-8.8s%09u%09u%02u%02u%8.8s%9.9s%03d%-20.20s%-19.19s%c%-124.124s%-40.40s%-40.40s",
           dmr_callsign.c_str(),     // Callsign (8 chars, left-aligned)
           dmr_rx_freq,              // RX Frequency (9 digits)
           dmr_tx_freq,              // TX Frequency (9 digits)
           power,                    // Power (2 digits)
           dmr_color_code,           // Color Code (2 digits)
           latitude,                 // Latitude (8 chars)
           longitude,                // Longitude (9 chars)
           height,                   // Height (3 digits)
           dmr_location.c_str(),     // Location (20 chars)
           dmr_description.c_str(),  // Description (19 chars)
           '4',                      // Slots (1 char: '4' = simplex)
           dmr_url.c_str(),          // URL (124 chars)
           firmwareVersion.c_str(),  // Version (40 chars) - from firmware variable
           "MMDVM_MMDVM_HS");        // Software (40 chars)

  // Debug: Log first 100 chars of config string
  String configDebug = "Config string preview: ";
  for (int i = 0; i < min(100, (int)strlen(configString)); i++) {
    if (configString[i] >= 32 && configString[i] <= 126) {
      configDebug += (char)configString[i];
    } else {
      configDebug += ".";
    }
  }
  logSerial(configDebug);
  logSerial("Config string length: " + String(strlen(configString)));

  // Build the packet with binary ID
  uint8_t configPacket[302];
  memset(configPacket, 0, sizeof(configPacket));

  memcpy(configPacket, "RPTC", 4);
  configPacket[4] = (id_to_send >> 24) & 0xFF;  // ID as 4 binary bytes (big-endian)
  configPacket[5] = (id_to_send >> 16) & 0xFF;
  configPacket[6] = (id_to_send >> 8) & 0xFF;
  configPacket[7] = id_to_send & 0xFF;
  memcpy(configPacket + 8, configString, 294);  // Copy exactly 294 bytes

  udp.beginPacket(dmr_server.c_str(), dmr_port);
  udp.write(configPacket, 302);
  udp.endPacket();

  logSerial("Config packet sent (302 bytes)");
}

void sendDMRKeepalive() {
  // Send keepalive/ping packet to DMR network
  // Format: "RPTPING" (7 bytes) + DMR_ID (4 bytes binary) = 11 bytes
  uint8_t keepalive[11];
  uint32_t id_to_send = dmr_id;

  if (dmr_essid > 0) {
    id_to_send = dmr_id * 100 + dmr_essid;
  }

  memcpy(keepalive, "RPTPING", 7);
  keepalive[7] = (id_to_send >> 24) & 0xFF;  // ID as 4 binary bytes (big-endian)
  keepalive[8] = (id_to_send >> 16) & 0xFF;
  keepalive[9] = (id_to_send >> 8) & 0xFF;
  keepalive[10] = id_to_send & 0xFF;

  udp.beginPacket(dmr_server.c_str(), dmr_port);
  udp.write(keepalive, 11);
  udp.endPacket();

  logSerialVerbose("Keepalive sent");
}

// ===== Serial Logging Functions =====
// Log to both USB serial and web buffer
void logSerial(String message) {
  Serial.println(message);

  // Store in circular buffer for web monitor
  serialLog[serialLogIndex] = message;
  serialLogIndex = (serialLogIndex + 1) % SERIAL_LOG_SIZE;
}

// Log with verbose flag - always to USB serial, conditionally to web buffer
void logSerialVerbose(String message) {
  // Always log to USB serial for debugging
  Serial.println(message);

  // Only store in web buffer if verbose logging is enabled
  if (verbose_logging) {
    serialLog[serialLogIndex] = message;
    serialLogIndex = (serialLogIndex + 1) % SERIAL_LOG_SIZE;
  }
}

// ===== Configuration Load/Save =====
void loadConfig() {
  preferences.begin("mmdvm", false);

  // Load DMR settings if they exist
  if (preferences.isKey("dmr_callsign")) {
    dmr_callsign = preferences.getString("dmr_callsign", DMR_CALLSIGN);
    dmr_id = preferences.getUInt("dmr_id", DMR_ID);
    dmr_server = preferences.getString("dmr_server", DMR_SERVER);
    dmr_password = preferences.getString("dmr_password", DMR_PASSWORD);
    dmr_essid = preferences.getUChar("dmr_essid", 0);

    // Load additional config options
    dmr_rx_freq = preferences.getUInt("dmr_rx_freq", 434000000);
    dmr_tx_freq = preferences.getUInt("dmr_tx_freq", 434000000);
    dmr_power = preferences.getUChar("dmr_power", 10);
    dmr_color_code = preferences.getUChar("dmr_cc", 1);
    dmr_latitude = preferences.getFloat("dmr_lat", 0.0);
    dmr_longitude = preferences.getFloat("dmr_lon", 0.0);
    dmr_height = preferences.getInt("dmr_height", 0);
    dmr_location = preferences.getString("dmr_location", "ESP32 Hotspot");
    dmr_description = preferences.getString("dmr_desc", "ESP32-MMDVM");
    dmr_url = preferences.getString("dmr_url", "");

    logSerial("Loaded DMR config from storage");
    logSerial("Callsign: " + dmr_callsign);
    logSerial("DMR ID: " + String(dmr_id));
    logSerial("Server: " + dmr_server);
    logSerial("ESSID: " + String(dmr_essid));
  }

  // Load WiFi alternate settings
  // Load alternate WiFi networks
  for (int i = 0; i < 5; i++) {
    String labelKey = "wifi" + String(i) + "_label";
    String ssidKey = "wifi" + String(i) + "_ssid";
    String passKey = "wifi" + String(i) + "_pass";
    wifiNetworks[i].label = preferences.getString(labelKey.c_str(), wifiNetworks[i].label);
    wifiNetworks[i].ssid = preferences.getString(ssidKey.c_str(), "");
    wifiNetworks[i].password = preferences.getString(passKey.c_str(), "");
  }

  // Load hostname setting
  device_hostname = preferences.getString("hostname", MDNS_HOSTNAME);
  logSerial("Hostname: " + device_hostname);

  // Load verbose logging setting
  verbose_logging = preferences.getBool("verbose_log", false);
  logSerial("Verbose logging: " + String(verbose_logging ? "enabled" : "disabled"));

  // Load web username
  web_username = preferences.getString("web_username", WEB_USERNAME);
  if (web_username.length() == 0) {
    web_username = WEB_USERNAME;  // Fallback to default if empty
  }

  // Load web password
  web_password = preferences.getString("web_password", WEB_PASSWORD);
  if (web_password.length() == 0) {
    web_password = WEB_PASSWORD;  // Fallback to default if empty
  }
  logSerial("Web authentication: enabled (user/password: " + web_username + "/" + web_password + ")");

  // Load mode enable/disable settings (use config.h defaults if not saved)
  mode_dmr_enabled = preferences.getBool("mode_dmr", DEFAULT_MODE_DMR);
  mode_dstar_enabled = preferences.getBool("mode_dstar", DEFAULT_MODE_DSTAR);
  mode_ysf_enabled = preferences.getBool("mode_ysf", DEFAULT_MODE_YSF);
  mode_p25_enabled = preferences.getBool("mode_p25", DEFAULT_MODE_P25);
  mode_nxdn_enabled = preferences.getBool("mode_nxdn", DEFAULT_MODE_NXDN);
  mode_pocsag_enabled = preferences.getBool("mode_pocsag", DEFAULT_MODE_POCSAG);
  logSerial("Mode status - DMR: " + String(mode_dmr_enabled ? "ON" : "OFF") + " | D-Star: " + String(mode_dstar_enabled ? "ON" : "OFF") + " | YSF: " + String(mode_ysf_enabled ? "ON" : "OFF") + " | P25: " + String(mode_p25_enabled ? "ON" : "OFF") + " | NXDN: " + String(mode_nxdn_enabled ? "ON" : "OFF") + " | POCSAG: " + String(mode_pocsag_enabled ? "ON" : "OFF"));

  preferences.end();
}

void saveConfig() {
  preferences.begin("mmdvm", false);

  preferences.putString("dmr_callsign", dmr_callsign);
  preferences.putUInt("dmr_id", dmr_id);
  preferences.putString("dmr_server", dmr_server);
  preferences.putString("dmr_password", dmr_password);
  preferences.putUChar("dmr_essid", dmr_essid);

  // Save additional config options
  preferences.putUInt("dmr_rx_freq", dmr_rx_freq);
  preferences.putUInt("dmr_tx_freq", dmr_tx_freq);
  preferences.putUChar("dmr_power", dmr_power);
  preferences.putUChar("dmr_cc", dmr_color_code);
  preferences.putFloat("dmr_lat", dmr_latitude);
  preferences.putFloat("dmr_lon", dmr_longitude);
  preferences.putInt("dmr_height", dmr_height);
  preferences.putString("dmr_location", dmr_location);
  preferences.putString("dmr_desc", dmr_description);
  preferences.putString("dmr_url", dmr_url);

  // Save alternate WiFi networks
  for (int i = 0; i < 5; i++) {
    String labelKey = "wifi" + String(i) + "_label";
    String ssidKey = "wifi" + String(i) + "_ssid";
    String passKey = "wifi" + String(i) + "_pass";
    preferences.putString(labelKey.c_str(), wifiNetworks[i].label);
    preferences.putString(ssidKey.c_str(), wifiNetworks[i].ssid);
    preferences.putString(passKey.c_str(), wifiNetworks[i].password);
  }

  // Save hostname
  preferences.putString("hostname", device_hostname);

  // Save verbose logging
  preferences.putBool("verbose_log", verbose_logging);

  // Save web credentials
  preferences.putString("web_username", web_username);
  preferences.putString("web_password", web_password);

  // Save mode enable/disable settings
  preferences.putBool("mode_dmr", mode_dmr_enabled);
  preferences.putBool("mode_dstar", mode_dstar_enabled);
  preferences.putBool("mode_ysf", mode_ysf_enabled);
  preferences.putBool("mode_p25", mode_p25_enabled);
  preferences.putBool("mode_nxdn", mode_nxdn_enabled);
  preferences.putBool("mode_pocsag", mode_pocsag_enabled);

  preferences.end();
  logSerial("Configuration saved to storage");
}

// ===== Web Server Setup =====
void setupWebServer() {
  // Main pages
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/serialmonitor", handleMonitor);
  server.on("/wificonfig", handleConfig);
  server.on("/modeconfig", handleDMRConfig);
  server.on("/admin", handleAdmin);

  // Configuration handlers
  server.on("/saveconfig", HTTP_POST, handleSaveConfig);
  server.on("/savedmrconfig", HTTP_POST, handleSaveDMRConfig);
  server.on("/savemodes", HTTP_POST, handleSaveModes);
  server.on("/resetconfig", handleResetConfig);
  server.on("/confirmreset", HTTP_POST, handleConfirmReset);

  // Data endpoints
  server.on("/logs", handleGetLogs);
  server.on("/wifiscan", handleWifiScan);

  // Admin actions
  server.on("/clearlogs", HTTP_POST, handleClearLogs);
  server.on("/save-hostname", HTTP_POST, handleSaveHostname);
  server.on("/save-verbose", HTTP_POST, handleSaveVerbose);
  server.on("/save-username", HTTP_POST, handleSaveUsername);
  server.on("/save-password", HTTP_POST, handleSavePassword);
  server.on("/reboot", HTTP_POST, handleReboot);
  server.on("/restart-services", HTTP_POST, handleRestartServices);
  server.on("/export-config", handleExportConfig);
  server.on(
    "/import-config", HTTP_POST, []() {}, handleImportConfig);
  server.on("/showprefs", handleShowPreferences);
  server.on("/test-mmdvm", HTTP_POST, handleTestMmdvm);
  server.on("/cleanup-prefs", HTTP_POST, handleCleanupPreferences);
  server.on("/download-update", HTTP_POST, handleDownloadUpdate);
  server.on(
    "/upload-firmware", HTTP_POST, []() {}, handleUploadFirmware);
  server.on("/flash-firmware", HTTP_POST, handleFlashFirmware);

  server.begin();
  logSerial("Web server started.");
}

// ===== Status LED Control =====
void setLEDMode(LED_MODE mode) {
  currentLedMode = mode;
  lastLedToggle = millis();

  // Set initial state based on mode
  if (mode == LED_MODE::STEADY) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    ledState = true;
  } else if (mode == LED_MODE::OFF) {
    digitalWrite(STATUS_LED_PIN, LOW);
    ledState = false;
  }
}

void updateStatusLED() {
  unsigned long currentMillis = millis();
  unsigned long interval;

  switch (currentLedMode) {
    case LED_MODE::OFF:
      // LED stays off
      break;

    case LED_MODE::STEADY:
      // LED stays on
      break;

    case LED_MODE::FAST_BLINK:
      // Fast blink (200ms interval) - connecting to WiFi
      interval = 200;
      if (currentMillis - lastLedToggle >= interval) {
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState ? HIGH : LOW);
        lastLedToggle = currentMillis;
      }
      break;

    case LED_MODE::SLOW_BLINK:
      // Slow blink (1000ms interval) - Access Point mode
      interval = 1000;
      if (currentMillis - lastLedToggle >= interval) {
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState ? HIGH : LOW);
        lastLedToggle = currentMillis;
      }
      break;
  }
}
