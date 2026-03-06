/*
 * Web Server System Module Implementation
 */

#include "system/system_webserver.h"
#include "system/system_eth.h"
#include "system/system_logger.h"
#include "system/system_oled.h"
#include "system/system_firmware.h"
#include "include/modem_flasher.h"
#include <HTTPClient.h>
#include <PubSubClient.h>
#include "web/pages/main.h"
#include "web/pages/mode_select.h"
#include "web/pages/mode_dmr.h"
#include "web/pages/mode_dstar.h"
#include "web/pages/mode_ysf.h"
#include "web/pages/mode_p25.h"
#include "web/pages/mode_nxdn.h"
#include "web/pages/mode_pocsag.h"
#include "web/pages/system_hotspot.h"
#include "web/pages/service_dapnet.h"
#include "web/pages/service_hampager.h"
#include "mmdvm/mmdvm_pocsag.h"
#include "mmdvm/mmdvm_dapnet.h"
#include "web/pages/system_serial.h"
#include "web/pages/system_status.h"
#include "web/pages/system_admin.h"
#include "web/pages/system_backup.h"
#include "web/pages/system_files.h"
#include "web/pages/system_firmware.h"
#include "web/pages/system_info.h"
#include "web/pages/service_mqtt.h"
#include "web/pages/service_wireguard.h"
#include "web/pages/system_sdcard.h"
#include "web/pages/system_database.h"
#include "web/pages/system_hardware.h"
#include "web/pages/system_network.h"
#include "web/pages/system_wifi.h"
#include "web/pages/system_modem.h"
#include "web/api/pocsag_send.h"
#include "include/sdcard_handlers.h"
#include <Update.h>
#include "web/include/touch_icon_png.h"
#include "web/include/pwa_icon_192_png.h"
#include "web/include/pwa_icon_512_png.h"
#include "system/web_handlers_ota.h"
#include "system/web_handlers_admin.h"
#include "system/web_handlers_config.h"
#include "system/web_handlers_nvs.h"
#include "system/web_handlers_dmr_settings.h"
#include "system/web_handlers_mqtt_settings.h"
#include "system/web_handlers_wg_settings.h"
#include "system/web_handlers_network_settings.h"
#include "system/web_handlers_hw_settings.h"
#include "system/web_handlers_radio.h"
#include "system/web_handlers_pocsag.h"
#include "system/web_handlers_wifi.h"
#include "system/web_handlers_snapshots.h"
#include "system/web_handlers_telegram_settings.h"
#include "web/pages/service_telegram.h"
#include "system/web_handlers_bootlogos.h"

extern "C"
{
#include <nvs_flash.h>
#include <nvs.h>
}

// External references to runtime settings (defined in main .ino)
extern bool modeDmrEnabled;
extern bool modeDstarEnabled;
extern bool modeYsfEnabled;
extern bool modeP25Enabled;
extern bool modeNxdnEnabled;
extern bool modePocsagEnabled;
extern bool dapnetEnabled;
extern uint32_t pocsagFrequency;
extern String dapnetServer;
extern uint16_t dapnetPort;
extern String dapnetNodeCs;
extern String dapnetAuthKey;
extern uint32_t dapnetRic;
extern String pocsagWhitelist;
extern String pocsagBlacklist;
extern String userCallsign;
extern uint32_t userDmrId;
extern uint8_t userDmrSsid;
extern uint32_t dmrRxFreq;
extern uint32_t dmrTxFreq;
extern uint8_t dmrColorCode;
extern uint8_t dmrRfPower;
extern String dmrServer;
extern uint16_t dmrPort;
extern uint16_t dmrLocalPort;
extern String dmrPassword;
extern uint16_t dmrHistorySize;
extern uint16_t dmrActivityTimeout;
extern uint16_t dmrUserCacheSize;
extern uint16_t dmrCallsignCacheSize;
extern uint16_t dmrApiTimeout;
extern String hotspotCallsign;
extern String hotspotSuffix;
extern String hotspotLatitude;
extern String hotspotLongitude;
extern int hotspotHeight;
extern String hotspotLocation;
extern String hotspotDescription;
extern String hotspotUrl;
extern String dmrApiUrl;
extern String qrzLookupUrl;
extern bool mqttEnabled;
extern String mqttBroker;
extern uint16_t mqttPort;
extern String mqttUser;
extern String mqttPassword;
extern String mqttStatusTopic;
extern String mqttLogsTopic;
extern String mqttHardwareTopic;

extern String mqttLoggerTaskTopic;
extern String mqttOledTaskTopic;
extern String mqttLedTaskTopic;
extern String mqttWifiTaskTopic;
extern String mqttEthTaskTopic;
extern String mqttArduinoOtaTaskTopic;
extern String mqttNtpTaskTopic;
extern String mqttMqttClientTaskTopic;
extern String mqttWebServerTaskTopic;
extern String mqttSdCardTaskTopic;
extern String mqttSensorTaskTopic;
extern String mqttWgTaskTopic;

extern String mqttModemTaskTopic;
extern String mqttDmrTaskTopic;
extern String mqttDstarTaskTopic;
extern String mqttYsfTaskTopic;
extern String mqttP25TaskTopic;
extern String mqttNxdnTaskTopic;
extern String mqttPocsagTaskTopic;

extern String mqttSubscribeTopic;
extern uint16_t mqttSendHardwareInfo;
extern bool mqttHardwareInfoLog;

extern bool wgEnabled;
extern String wgLocalIp;
extern String wgPrivateKey;
extern String wgPublicKey;
extern String wgEndpoint;
extern uint16_t wgEndpointPort;
extern String wgDns;
extern String wgAllowedIps;
extern bool wireguardConnected;

extern bool mdnsEnabled;
extern String mdnsHostname;
extern bool ntpEnabled;
extern String ntpServer;
extern int32_t ntpGmtOffsetSec;
extern int32_t ntpDaylightOffsetSec;
extern uint32_t ntpSyncIntervalMs;
extern bool webEnabled;
extern uint16_t webServerPort;
extern String webUsername;
extern String webPassword;
extern String wifiSsid;
extern String wifiPassword;
#define WIFI_SLOT_COUNT 6
extern String wifiSlotLabel[WIFI_SLOT_COUNT];
extern String wifiSlotSsid[WIFI_SLOT_COUNT];
extern String wifiSlotPass[WIFI_SLOT_COUNT];
extern String wifiApSsid;
extern String wifiApPassword;
extern uint8_t wifiApChannel;
extern uint8_t wifiMaxRetries;
extern bool ethEnabled;
extern bool ethDebug;
extern bool dnsFallbackEnabled;
extern String dnsFallbackIp;
extern int ledPin;
extern int buttonPin;
extern bool oledEnabled;
extern int i2cSdaPin;
extern int i2cSclPin;
extern int oledI2cAddress;
extern int oledWidth;
extern int oledHeight;

extern void saveSettings();


TaskHandle_t webServerTaskHandle = NULL;
// Constructed with compile-time default; reconstructed in webServerTask() using
// placement new after loadSettings() has populated webServerPort from NVS.
extern uint16_t webServerPort;
WebServer server(webServerPort);

// Apple touch icon — 180x180 PNG generated from the SVG antenna icon
// Stored in PROGMEM (web/include/touch_icon_png.h), served to iOS devices
static void serveTouchIconPNG()
{
  server.sendHeader("Cache-Control", "max-age=86400");
  server.send_P(200, "image/png",
                (const char *)TOUCH_ICON_PNG, TOUCH_ICON_PNG_LEN);
}

// Favicon SVG served at /favicon.ico
//static const char FAVICON_SVG[] PROGMEM = R"rawsvg(<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'><rect width='64' height='64' rx='12' fill='#222'/><path d='M32 18v28' stroke='#00bfff' stroke-width='5' stroke-linecap='round'/><path d='M22 28a14 14 0 0 1 20 0' stroke='#00bfff' stroke-width='4' fill='none' stroke-linecap='round'/><path d='M16 22a22 22 0 0 1 32 0' stroke='#00bfff' stroke-width='3.5' fill='none' stroke-linecap='round'/><circle cx='32' cy='50' r='5' fill='#00bfff'/></svg>)rawsvg";
static const char FAVICON_SVG[] PROGMEM = R"rawsvg(<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 40 40'><g><path fill='#010101' d='M22.603,19.604c1.136-0.807,1.88-2.131,1.88-3.626c0-2.452-1.994-4.446-4.444-4.446c-2.451,0-4.445,1.994-4.445,4.446c0,1.495,0.745,2.82,1.881,3.627l-5.008,14.432c-0.132,0.378,0.068,0.791,0.447,0.924C12.991,34.986,13.072,35,13.15,35c0.3,0,0.581-0.188,0.684-0.488l1.02-2.933h10.37l1.017,2.933C26.344,34.813,26.626,35,26.927,35c0.078,0,0.157-0.014,0.237-0.039c0.378-0.133,0.578-0.546,0.446-0.924L22.603,19.604z M20.038,12.991c1.646,0,2.986,1.34,2.986,2.987c0,1.646-1.34,2.986-2.986,2.986c-1.647,0-2.986-1.34-2.986-2.986C17.052,14.331,18.391,12.991,20.038,12.991z M20.038,20.423c0.435,0,0.854-0.064,1.252-0.181l1.603,4.622h-5.709l1.604-4.622C19.185,20.359,19.604,20.423,20.038,20.423z M15.356,30.129l1.324-3.814h6.715l1.325,3.814H15.356z'/><path fill='#010101' d='M14.12,21.617c0.142,0.142,0.327,0.212,0.511,0.212c0.187,0,0.373-0.07,0.515-0.212c0.282-0.283,0.282-0.743,0-1.025c-1.234-1.231-1.913-2.87-1.913-4.613c0-1.743,0.678-3.381,1.911-4.613c0.282-0.283,0.282-0.742,0-1.026c-0.285-0.282-0.743-0.282-1.025,0c-1.507,1.506-2.336,3.508-2.336,5.639C11.782,18.108,12.612,20.111,14.12,21.617z'/><path fill='#010101' d='M12.581,23.154c-1.919-1.916-2.974-4.465-2.974-7.176c0-2.71,1.055-5.259,2.973-7.178c0.283-0.283,0.283-0.742,0-1.025c-0.284-0.283-0.743-0.282-1.025,0.001c-2.191,2.191-3.398,5.104-3.398,8.202s1.208,6.011,3.4,8.202c0.142,0.142,0.326,0.212,0.512,0.212s0.372-0.07,0.513-0.212C12.864,23.896,12.864,23.438,12.581,23.154z'/><path fill='#010101' d='M5.982,15.979c0-3.678,1.432-7.137,4.034-9.741c0.283-0.283,0.283-0.742,0-1.025c-0.283-0.283-0.743-0.283-1.025,0c-2.876,2.877-4.46,6.7-4.46,10.766c0,4.065,1.584,7.889,4.46,10.766c0.142,0.141,0.327,0.211,0.513,0.211s0.372-0.07,0.513-0.211c0.283-0.284,0.283-0.744,0-1.026C7.415,23.115,5.982,19.656,5.982,15.979z'/><path fill='#010101' d='M26.768,15.979c0,1.742-0.679,3.38-1.911,4.612c-0.282,0.282-0.282,0.741,0,1.024c0.142,0.142,0.328,0.213,0.514,0.213c0.185,0,0.37-0.071,0.512-0.213c1.507-1.506,2.337-3.508,2.337-5.637c0-2.131-0.831-4.134-2.338-5.64c-0.283-0.283-0.741-0.282-1.026,0c-0.282,0.283-0.282,0.742,0.001,1.025C26.089,12.596,26.768,14.235,26.768,15.979z'/><path fill='#010101' d='M28.445,7.775c-0.285-0.283-0.744-0.282-1.026,0.001c-0.284,0.283-0.284,0.742,0,1.024c1.917,1.917,2.973,4.466,2.973,7.178c0,2.709-1.055,5.258-2.971,7.178c-0.284,0.282-0.284,0.74,0,1.024c0.142,0.142,0.328,0.212,0.513,0.212c0.186,0,0.372-0.07,0.513-0.212c2.19-2.192,3.397-5.105,3.397-8.202C31.844,12.879,30.636,9.966,28.445,7.775z'/><path fill='#010101' d='M31.007,5.212c-0.282-0.282-0.741-0.282-1.025,0c-0.283,0.283-0.283,0.743,0,1.025c2.603,2.603,4.037,6.063,4.037,9.741c0,3.677-1.434,7.136-4.035,9.74c-0.283,0.283-0.283,0.741,0,1.025c0.141,0.141,0.327,0.211,0.513,0.211c0.185,0,0.371-0.07,0.513-0.211c2.876-2.879,4.459-6.701,4.459-10.766C35.468,11.912,33.884,8.089,31.007,5.212z'/></g></svg>)rawsvg";

// Shared statistics
int requestCount = 0;
volatile float cpuUsage = 0;

void initWebServerTask()
{
  BaseType_t result = xTaskCreatePinnedToCore(
      webServerTask,
      "Web Server Task",
      WEBSERVER_TASK_STACK,
      NULL,
      WEBSERVER_TASK_PRIORITY,
      &webServerTaskHandle,
      0 // Run on core 0 (HTTP is not time-critical)
  );
  if (result != pdPASS)
    log_e("[WebServer] Task creation FAILED! Free heap: %u", ESP.getFreeHeap());
}

// Forward declarations for page handlers
void handleServiceWireguard();
void handleServiceTelegram();
void handleSystemDatabase();

void webServerTask(void *parameter) {
    // The global server was constructed at static-init time using WEB_SERVER_PORT.
    // Reconstruct it now with the NVS-loaded port. begin() has not been called yet,
    // so no socket is open and placement new is safe.
    server.~WebServer();
    new (&server) WebServer(webServerPort);
    addLogMessage("[WebServer Task] Listening on port " + String(webServerPort));
  registerRadioRoutes();
  registerPocsagRoutes();
  registerWifiRoutes();
  // Global auth middleware - protects all routes when webEnabled is true
  server.addMiddleware([](WebServer &srv, Middleware::Callback next) -> bool {
    if (webEnabled && !srv.authenticate(webUsername.c_str(), webPassword.c_str())) {
      srv.requestAuthentication();
      return true;
    }
    return next();
  });

  addLogMessage("[WebServer Task] Started");

  // Wait for any network connection (WiFi OR Ethernet)
  while (WiFi.status() != WL_CONNECTED && !ethConnected)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  addLogMessage("[WebServer Task] Network available - starting server");


  server.on("/favicon.ico", []()
            {
              server.sendHeader("Cache-Control", "no-store");
              server.send(200, "image/svg+xml", FPSTR(FAVICON_SVG));
            });
  server.on("/apple-touch-icon.png", []() { serveTouchIconPNG(); });
  server.on("/pwa-icon-192.png", []() {
    server.sendHeader("Cache-Control", "max-age=86400");
    server.send_P(200, "image/png", (const char *)PWA_ICON_192_PNG, PWA_ICON_192_PNG_LEN);
  });
  server.on("/pwa-icon-512.png", []() {
    server.sendHeader("Cache-Control", "max-age=86400");
    server.send_P(200, "image/png", (const char *)PWA_ICON_512_PNG, PWA_ICON_512_PNG_LEN);
  });
  server.on("/manifest.json", []() {
    extern String userCallsign;
    extern String mdnsHostname;
    String json = "{";
    json += "\"name\":\"" + userCallsign + " MMDVM\",";
    json += "\"short_name\":\"" + userCallsign + "\",";
    json += "\"start_url\":\"/\",";
    json += "\"display\":\"standalone\",";
    json += "\"background_color\":\"#333333\",";
    json += "\"theme_color\":\"#333333\",";
    json += "\"icons\":[";
    json += "{\"src\":\"/apple-touch-icon.png\",\"sizes\":\"180x180\",\"type\":\"image/png\"},";
    json += "{\"src\":\"/pwa-icon-192.png\",\"sizes\":\"192x192\",\"type\":\"image/png\"},";
    json += "{\"src\":\"/pwa-icon-512.png\",\"sizes\":\"512x512\",\"type\":\"image/png\"}";
    json += "]}";
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", json);
  });
  server.on("/", handleRoot);
  server.on("/mode-select", handleModeSelect);
  server.on("/mode-dmr", handleModeDmr);
  server.on("/mode-dstar", handleModeDstar);
  server.on("/mode-ysf", handleModeYsf);
  server.on("/mode-p25", handleModeP25);
  server.on("/mode-nxdn", handleModeNxdn);
  server.on("/mode-pocsag", handleModePocsag);
  server.on("/system-hotspot", handleSystemHotspot);
  server.on("/service-dapnet", handleServiceDapnet);
  server.on("/service-hampager", handleServiceHampager);
  server.on("/system-serialmonitor", handleSystemSerial);
  server.on("/system-status", handleSystemStatus);
  server.on("/system-info", handleSystemInfo);
  server.on("/system-hardware", handleSystemHardware);
  server.on("/system-network", handleSystemNetwork);
  server.on("/system-modem", handleSystemModem);
  server.on("/system-wifi", handleSystemWifi);
  server.on("/service-mqtt", handleServiceMqtt);
  server.on("/service-telegram", handleServiceTelegram);
  server.on("/service-wireguard", handleServiceWireguard);
  server.on("/system-sdcard", handleSystemSdcard);
  server.on("/system-database", handleSystemDatabase);
  server.on("/system-firmware", handleSystemFirmware);
  server.on("/system-admin", handleSystemAdmin);
  server.on("/system-backup", handleSystemBackup);
  server.on("/system-files", handleSystemFiles);

  registerOtaRoutes();

  registerDmrSettingsRoutes();
  registerMqttSettingsRoutes();
  registerTelegramSettingsRoutes();
  registerWgSettingsRoutes();
  registerNetworkSettingsRoutes();
  registerHwSettingsRoutes();

  registerAdminRoutes();
  registerConfigRoutes();
  registerNvsRoutes();
  registerSnapshotRoutes();
  registerBootlogosRoutes();

  server.onNotFound(handleNotFound);
  server.begin(webServerPort);
  addLogMessage("[WebServer Task] Server started on port " + String(webServerPort));
  while (true)
  {
    server.handleClient();

    // Process SD card downloads if requested
    performCSVDownload();
    performSQLiteDownload();
    performBootlogosInstall();

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void handleRoot()
{
  requestCount++;
  String html = getMainPageHTML(requestCount, cpuUsage, ESP.getHeapSize());
  server.send(200, "text/html", html);
  //Serial.println("[WebServer] Root page served");
  addLogMessage("[WebServer] Root page served");
}

void handleNotFound()
{
  server.send(404, "text/plain", "404: Not Found");
}

void handleModeSelect()
{
  requestCount++;
  server.send(200, "text/html", getModeSelectPageHTML());
}

void handleModeDmr()
{
  requestCount++;
  server.send(200, "text/html", getModeDmrPageHTML());
}

void handleModeDstar()
{
  requestCount++;
  server.send(200, "text/html", getModeDstarPageHTML());
}

void handleModeYsf()
{
  requestCount++;
  server.send(200, "text/html", getModeYsfPageHTML());
}

void handleModeP25()
{
  requestCount++;
  server.send(200, "text/html", getModeP25PageHTML());
}

void handleModeNxdn()
{
  requestCount++;
  server.send(200, "text/html", getModeNxdnPageHTML());
}

void handleModePocsag()
{
  requestCount++;
  server.send(200, "text/html", getModePocsagPageHTML());
}

void handleServiceDapnet()
{
  requestCount++;
  server.send(200, "text/html", getServiceDapnetPageHTML());
}

void handleServiceHampager()
{
  requestCount++;
  server.send(200, "text/html", getServiceHampagerPageHTML());
}

void handleSystemSerial()
{
  requestCount++;
  server.send(200, "text/html", getSystemSerialPageHTML());
}

void handleSystemStatus()
{
  requestCount++;
  server.send(200, "text/html", getSystemStatusPageHTML());
}

void handleSystemInfo()
{
  requestCount++;
  server.send(200, "text/html", getSystemInfoPageHTML());
}

void handleSystemHardware()
{
  requestCount++;
  server.send(200, "text/html", getSystemHardwarePageHTML());
}

void handleSystemNetwork()
{
  requestCount++;
  server.send(200, "text/html", getSystemNetworkPageHTML());
}

void handleSystemHotspot()
{
  requestCount++;
  server.send(200, "text/html", getSystemHotspotPageHTML());
}

void handleSystemModem()
{
  requestCount++;
  server.send(200, "text/html", getSystemModemPageHTML());
}

void handleSystemWifi()
{
  requestCount++;
  server.send(200, "text/html", getSystemWifiPageHTML());
}

void handleServiceMqtt()
{
  requestCount++;
  server.send(200, "text/html", getServiceMqttPageHTML());
}

void handleServiceTelegram()
{
  requestCount++;
  server.send(200, "text/html", getServiceTelegramPageHTML());
}

void handleServiceWireguard()
{
  requestCount++;
  server.send(200, "text/html", getServiceWireguardPageHTML());
}

void handleSystemSdcard()
{
  requestCount++;
  server.send(200, "text/html", getSystemSdcardPageHTML());
}

void handleSystemDatabase()
{
  requestCount++;
  server.send(200, "text/html", getSystemDatabasePageHTML());
}

void handleSystemFirmware()
{
  requestCount++;
  server.send(200, "text/html", getSystemFirmwarePageHTML());
}

void handleSystemAdmin()
{
  requestCount++;
  server.send(200, "text/html", getSystemAdminPageHTML());
}

void handleSystemBackup()
{
  requestCount++;
  server.send(200, "text/html", getSystemBackupPageHTML());
}

void handleSystemFiles()
{
  requestCount++;
  server.send(200, "text/html", getSystemFilesPageHTML());
}
