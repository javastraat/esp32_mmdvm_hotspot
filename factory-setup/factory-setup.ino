/*
 * ESP32 MMDVM Hotspot - Factory Setup
 *
 * Minimal bootstrap firmware for initial deployment
 * Connects to WiFi or creates AP, then allows OTA update to full firmware
 *
 * by PD2EMC & PD8JO
 */

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Preferences.h>
#include "config.h"
#include "webpages.h"

WebServer server(80);
Preferences preferences;
bool apMode = false;
String currentIP = "";
String savedSSID = "";
String savedPassword = "";

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n===========================================");
  Serial.println("ESP32 MMDVM Hotspot - Factory Setup");
  Serial.println("Version: " + String(FACTORY_VERSION));
  Serial.println("===========================================\n");

  // Clear ESP32's internal WiFi credentials to prevent auto-connect to old networks
  WiFi.disconnect(true);  // true = erase stored WiFi credentials from flash
  delay(100);

  // Open preferences and load saved WiFi credentials (using same namespace as main firmware)
  preferences.begin("mmdvm", false); // false = read-write mode (correct for saving credentials)
  
  // Debug: Print all preferences in the namespace
  Serial.println("=== DEBUG: Preferences namespace 'mmdvm' contents ===");
  size_t freeEntries = preferences.freeEntries();
  Serial.println("Free preference entries: " + String(freeEntries));
  
  savedSSID = preferences.getString("ssid", "");
  savedPassword = preferences.getString("password", "");
  
  Serial.println("Stored SSID: '" + savedSSID + "'");
  Serial.println("Stored Password: '" + savedPassword + "'");
  Serial.println("SSID length: " + String(savedSSID.length()));
  Serial.println("Password length: " + String(savedPassword.length()));
  Serial.println("=== End preferences debug ===");

  String wifiSSID = WIFI_SSID;
  String wifiPassword = WIFI_PASSWORD;

  // Use saved WiFi if available, otherwise use default from config.h
  if (savedSSID.length() > 0) {
    wifiSSID = savedSSID;
    wifiPassword = savedPassword;
    Serial.println("Found saved WiFi credentials in Preferences");
    Serial.println("Attempting to connect to saved WiFi: " + wifiSSID);
  } else {
    Serial.println("No saved WiFi found in Preferences, using default from config");
    Serial.println("Attempting to connect to WiFi: " + wifiSSID);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < WIFI_TIMEOUT_SECONDS) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    // Successfully connected to WiFi
    Serial.println("\n\nWiFi connected!");
    Serial.println("IP address: " + WiFi.localIP().toString());
    currentIP = WiFi.localIP().toString();
    apMode = false;
  } else {
    // Failed to connect, start AP mode
    Serial.println("\n\nWiFi connection failed. Starting Access Point mode...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);

    currentIP = WiFi.softAPIP().toString();
    Serial.println("Access Point started!");
    Serial.println("SSID: " + String(AP_SSID));
    Serial.println("Password: " + String(AP_PASSWORD));
    Serial.println("IP address: " + currentIP);
    apMode = true;
  }

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/scan-networks", handleScanNetworks);
  server.on("/connect-wifi", HTTP_POST, handleConnectWiFi);
  server.on("/download-update", HTTP_POST, handleDownloadUpdate);
  server.on("/upload-firmware", HTTP_POST, []() {}, handleUploadFirmware);
  server.on("/flash-firmware", HTTP_POST, handleFlashFirmware);

  server.begin();
  Serial.println("\nWeb server started!");
  Serial.println("Navigate to: http://" + currentIP);
  Serial.println("\nReady for OTA firmware update.\n");
}

void loop() {
  server.handleClient();
  delay(1);
}

void handleDownloadUpdate() {
  // Get version parameter (default to stable if not specified)
  String version = "stable";
  if (server.hasArg("version")) {
    version = server.arg("version");
  }

  // Select appropriate URL based on version
  String downloadUrl;
  if (version == "beta") {
    downloadUrl = OTA_UPDATE_BETA_URL;
    Serial.println("Starting BETA firmware download from GitHub...");
  } else {
    downloadUrl = OTA_UPDATE_URL;
    Serial.println("Starting stable firmware download from GitHub...");
  }

  HTTPClient http;
  http.begin(downloadUrl);
  http.setTimeout(OTA_TIMEOUT);

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();

    if (contentLength > 0) {
      Serial.println("Firmware download successful, size: " + String(contentLength) + " bytes");

      // Check if there's enough space
      if (Update.begin(contentLength)) {
        WiFiClient *client = http.getStreamPtr();
        size_t written = Update.writeStream(*client);

        if (written == contentLength) {
          if (Update.end(true)) {
            Serial.println("Firmware downloaded and finalized successfully: " + String(contentLength) + " bytes");
            server.send(200, "text/plain", "SUCCESS: Firmware ready for flash (" + String(contentLength) + " bytes)");
          } else {
            Serial.println("Failed to finalize downloaded firmware - Error: " + String(Update.getError()));
            server.send(500, "text/plain", "ERROR: Failed to finalize firmware - " + String(Update.getError()));
          }
        } else {
          Update.abort();
          Serial.println("Download incomplete: " + String(written) + " of " + String(contentLength) + " bytes");
          server.send(500, "text/plain", "ERROR: Download incomplete");
        }
      } else {
        Serial.println("Not enough space for firmware update");
        server.send(500, "text/plain", "ERROR: Not enough space for update");
      }
    } else {
      Serial.println("Invalid firmware size from server");
      server.send(500, "text/plain", "ERROR: Invalid firmware file");
    }
  } else {
    Serial.println("Failed to download firmware, HTTP code: " + String(httpCode));
    server.send(500, "text/plain", "ERROR: Failed to download (HTTP " + String(httpCode) + ")");
  }

  http.end();
}

void handleUploadFirmware() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.println("Starting firmware upload: " + upload.filename);

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Serial.println("Failed to begin OTA update");
      return;
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Serial.println("OTA write failed");
      Update.abort();
      return;
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.println("Firmware upload successful: " + String(upload.totalSize) + " bytes");
      server.send(200, "text/plain", "SUCCESS: Firmware ready for flash (" + String(upload.totalSize) + " bytes)");
    } else {
      Serial.println("Upload failed: " + String(Update.getError()));
      server.send(500, "text/plain", "ERROR: Upload failed - " + String(Update.getError()));
    }
  }
}

void handleFlashFirmware() {
  Serial.println("Starting firmware flash process...");

  // Check if firmware was properly prepared by either download or upload method
  if (Update.isFinished()) {
    Serial.println("Firmware flash completed successfully!");
    server.send(200, "text/plain", "SUCCESS: Firmware flashed, rebooting...");

    delay(2000);
    ESP.restart();
  } else {
    Serial.println("No firmware ready for flashing - Update.isFinished() = false");
    server.send(400, "text/plain", "ERROR: No firmware prepared for flash");
  }
}

void handleConnectWiFi() {
  // Get WiFi credentials from POST request
  if (!server.hasArg("ssid") || !server.hasArg("password")) {
    server.send(400, "text/plain", "ERROR: Missing SSID or password");
    return;
  }

  String ssid = server.arg("ssid");
  String password = server.arg("password");

  Serial.println("Attempting to connect to new WiFi network: " + ssid);

  // Clear ESP32's internal WiFi credentials and disconnect from AP
  WiFi.disconnect(true);  // true = erase stored WiFi credentials from flash
  delay(100);

  // Switch to STA mode and connect to new network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  // Wait for connection (max 20 seconds)
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    // Successfully connected
    String newIP = WiFi.localIP().toString();
    Serial.println("\n\nWiFi connected successfully!");
    Serial.println("New IP address: " + newIP);

    // Save WiFi credentials to preferences
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    Serial.println("WiFi credentials saved to preferences");

    // Update global variables
    currentIP = newIP;
    apMode = false;
    savedSSID = ssid;
    savedPassword = password;

    server.send(200, "text/plain", "SUCCESS: Connected to WiFi\nIP: " + newIP);
  } else {
    // Failed to connect, restart AP mode
    Serial.println("\n\nFailed to connect to WiFi network: " + ssid);
    Serial.println("Restarting Access Point mode...");

    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    currentIP = WiFi.softAPIP().toString();
    apMode = true;

    server.send(500, "text/plain", "ERROR: Failed to connect to WiFi network\nPlease check your SSID and password and try again");
  }
}

void handleScanNetworks() {
  Serial.println("Starting WiFi network scan...");

  // Scan for networks
  int n = WiFi.scanNetworks();

  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    json += "\"encryption\":" + String(WiFi.encryptionType(i));
    json += "}";
  }
  json += "]";

  Serial.println("Found " + String(n) + " networks");
  server.send(200, "application/json", json);

  // Clean up
  WiFi.scanDelete();
}
