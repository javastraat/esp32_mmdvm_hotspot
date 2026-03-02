/*
 * ============================================================================
 * ESP32 RTOS MMDVM - Multi-Mode Digital Voice Modem
 * ============================================================================
 *
 * FreeRTOS-based implementation supporting multiple digital voice protocols:
 * DMR, D-STAR, System Fusion (YSF), P25, NXDN, and POCSAG
 *
 * ARCHITECTURE:
 *   • Modular task-based design with independent timing and priorities
 *   • Each protocol and system component runs as a separate RTOS task
 *   • Web-based configuration and monitoring interface
 *   • Dual network support: WiFi and Ethernet
 *   • Integrated logging, OLED display, and SD card support
 *
 * RTOS BENEFITS:
 *   • True multitasking - protocols and systems run concurrently
 *   • No blocking operations - everything remains responsive
 *   • Priority-based scheduling for time-critical operations
 *   • Efficient CPU usage with automatic sleep during idle
 *
 * PROJECT STRUCTURE:
 *   system/     System components (WiFi, Ethernet, logging, display)
 *   mmdvm/      Digital voice protocol implementations
 *   web/        Web interface pages and assets
 *   include/    Configuration headers
 *
 * ============================================================================

System/Hardware Tasks:

Logger task (initLogger)
OLED display task (initOledTask)
LED status task (initLedTask)
WiFi task (initWiFiTask)
Ethernet task (initEthTask)
Arduino OTA update task (initArduinoOtaTask)
NTP time sync task (initNtpTask)
MQTT client task (initMqttTask)
Web server task (initWebServerTask)
SD card task (initSdCardTask)
(Optional) Sensor monitoring task (initSensorTask, currently commented out)
WireGuard VPN task (initWireguardTask)
MMDVM/Protocol Tasks:

Modem task (initModemTask)
DMR protocol task (initDmrTask)
D-STAR protocol task (initDstarTask)
YSF protocol task (initYsfTask)
P25 protocol task (initP25Task)
NXDN protocol task (initNxdnTask)
POCSAG protocol task (initPocsagTask)
*/

// Configuration
#include "include/config.h"
#include <Preferences.h>
#include <LittleFS.h>

// System modules
#include "system/system_wifi.h"
#include "system/system_eth.h"
#include "system/system_webserver.h"
#include "system/system_oled.h"
#include "system/system_led.h"
#include "system/system_sensor.h"
#include "system/system_sdcard.h"
#include "system/system_logger.h"
#include "system/system_ntp.h"
#include "system/service_mqtt.h"
#include "system/service_telegram.h"
#include "system/system_firmware.h"
#include "system/system_modem.h"
#include "system/system_arduinoota.h"
#include "system/service_wireguard.h"


// MMDVM Protocol modules
#include "mmdvm/mmdvm_dmr.h"
#include "mmdvm/mmdvm_dstar.h"
#include "mmdvm/mmdvm_ysf.h"
#include "mmdvm/mmdvm_p25.h"
#include "mmdvm/mmdvm_nxdn.h"
#include "mmdvm/mmdvm_pocsag.h"
#include "mmdvm/mmdvm_dapnet.h"

// Preferences for persistent storage
Preferences preferences;

// Runtime settings (loaded from NVS or defaults from config.h)
bool modeDmrEnabled = DEFAULT_MODE_DMR;
bool modeDstarEnabled = DEFAULT_MODE_DSTAR;
bool modeYsfEnabled = DEFAULT_MODE_YSF;
bool modeP25Enabled = DEFAULT_MODE_P25;
bool modeNxdnEnabled = DEFAULT_MODE_NXDN;
bool modePocsagEnabled = DEFAULT_MODE_POCSAG;
bool dapnetEnabled     = DAPNET_ENABLED;

// CW ID Settings (loaded from NVS or defaults from config.h)
bool cwidEnabled = CWID_ENABLED;
uint8_t cwidIntervalMin = CWID_INTERVAL_MIN;

// DAPNET / POCSAG Network Settings (loaded from NVS or defaults from config.h)
uint32_t pocsagFrequency = POCSAG_FREQUENCY;
String dapnetServer      = DAPNET_SERVER;
uint16_t dapnetPort      = DAPNET_PORT;
String dapnetNodeCs      = DAPNET_NODE_CS;
String dapnetAuthKey     = DAPNET_AUTH_KEY;
uint32_t dapnetRic       = 0; // 0 = use userDmrId at runtime
String pocsagWhitelist   = POCSAG_WHITELIST;
String pocsagBlacklist   = POCSAG_BLACKLIST;
String hampagerUser      = HAMPAGER_USER;
String hampagerPassword  = HAMPAGER_PASSWORD;
String hampagerTxGroup   = HAMPAGER_TXG;
String userCallsign = DMR_CALLSIGN;
uint32_t userDmrId = DMR_ID;
uint8_t userDmrSsid = DMR_SSID;

// DMR Network Settings (loaded from NVS or defaults from config.h)
uint32_t dmrRxFreq = DMR_RX_FREQ;
uint32_t dmrTxFreq = DMR_TX_FREQ;
uint8_t dmrColorCode = DMR_COLOR_CODE;
uint8_t dmrRfPower = DMR_RF_POWER;

// DMR Server Settings (loaded from NVS or defaults from config.h)
String dmrServer = DMR_SERVER;
uint16_t dmrPort = DMR_PORT;
uint16_t dmrLocalPort = DMR_LOCAL_PORT;
String dmrPassword = DMR_PASSWORD;
uint16_t dmrHistorySize = DMR_HISTORY_SIZE;
uint16_t dmrActivityTimeout = DMR_ACTIVITY_TIMEOUT;
uint16_t dmrUserCacheSize = DMR_USER_CACHE_SIZE;
uint16_t dmrCallsignCacheSize = DMR_CALLSIGN_CACHE_SIZE;
uint16_t dmrApiTimeout = DMR_API_TIMEOUT;

// Hotspot Settings (loaded from NVS or defaults from config.h)
String hotspotCallsign = HOTSPOT_CALLSIGN;
String hotspotSuffix = HOTSPOT_SUFFIX;
String hotspotLatitude = HOTSPOT_LATITUDE;
String hotspotLongitude = HOTSPOT_LONGITUDE;
int hotspotHeight = HOTSPOT_HEIGHT;
String hotspotLocation = HOTSPOT_LOCATION;
String hotspotDescription = HOTSPOT_DESCRIPTION;
String hotspotUrl = HOTSPOT_URL;

// DMR API Settings (loaded from NVS or defaults from config.h)
String dmrApiUrl = DMR_API_URL;
String qrzLookupUrl = QRZ_LOOKUP_URL;

// WiFi Station Settings - 6 slots (loaded from NVS or defaults from config.h)
#define WIFI_SLOT_COUNT 6
String wifiSlotLabel[WIFI_SLOT_COUNT] = {WIFI_SLOT_LABEL, WIFI_SLOT1_LABEL, WIFI_SLOT2_LABEL, WIFI_SLOT3_LABEL, WIFI_SLOT4_LABEL, WIFI_SLOT5_LABEL};
String wifiSlotSsid[WIFI_SLOT_COUNT] = {WIFI_SSID, WIFI_SSID1, WIFI_SSID2, WIFI_SSID3, WIFI_SSID4, WIFI_SSID5};
String wifiSlotPass[WIFI_SLOT_COUNT] = {WIFI_PASSWORD, WIFI_PASSWORD1, WIFI_PASSWORD2, WIFI_PASSWORD3, WIFI_PASSWORD4, WIFI_PASSWORD5};
// Backward-compatible aliases for slot 0 (used by webserver export/import)
String &wifiSsid = wifiSlotSsid[0];
String &wifiPassword = wifiSlotPass[0];

// WiFi AP Settings (loaded from NVS or defaults from config.h)
String wifiApSsid = WIFI_AP_SSID;
String wifiApPassword = WIFI_AP_PASSWORD;
uint8_t wifiApChannel = WIFI_AP_CHANNEL;
uint8_t wifiMaxRetries = WIFI_MAX_RETRIES;

// Ethernet Settings (loaded from NVS or defaults from config.h)
bool ethEnabled = ETH_ENABLED;
bool ethDebug = ETH_DEBUG;

// DNS Settings (loaded from NVS or defaults from config.h)
bool dnsFallbackEnabled = DNS_FALLBACK_ENABLED;
String dnsFallbackIp = DNS_FALLBACK_IP;

// mDNS Settings (loaded from NVS or defaults from config.h)
bool mdnsEnabled = ENABLE_MDNS;
String mdnsHostname = MDNS_HOSTNAME;

// NTP Settings (loaded from NVS or defaults from config.h)
bool ntpEnabled = NTP_ENABLED;
String ntpServer = NTP_SERVER;
int32_t ntpGmtOffsetSec = NTP_GMT_OFFSET_SEC;
int32_t ntpDaylightOffsetSec = NTP_DAYLIGHT_OFFSET_SEC;
uint32_t ntpSyncIntervalMs = NTP_SYNC_INTERVAL_MS;

// MQTT Settings (loaded from NVS or defaults from config.h)
bool mqttEnabled = MQTT_ENABLED;
String mqttBroker = MQTT_BROKER;
uint16_t mqttPort = MQTT_PORT;
String mqttUser = MQTT_USER;
String mqttPassword = MQTT_PASSWORD;
String mqttStatusTopic = MQTT_STATUS_TOPIC;
String mqttLogsTopic = MQTT_LOGS_TOPIC;
String mqttHardwareTopic = MQTT_HARDWARE_TOPIC;
String mqttLoggerTaskTopic = MQTT_LOGGER_TASK_TOPIC;
String mqttOledTaskTopic = MQTT_OLED_TASK_TOPIC;
String mqttLedTaskTopic = MQTT_LED_TASK_TOPIC;
String mqttWifiTaskTopic = MQTT_WIFI_TASK_TOPIC;
String mqttEthTaskTopic = MQTT_ETH_TASK_TOPIC;
String mqttArduinoOtaTaskTopic = MQTT_ARDUINO_OTA_TASK_TOPIC;
String mqttNtpTaskTopic = MQTT_NTP_TASK_TOPIC;
String mqttMqttClientTaskTopic = MQTT_MQTT_CLIENT_TASK_TOPIC;
String mqttWebServerTaskTopic = MQTT_WEB_SERVER_TASK_TOPIC;
String mqttSdCardTaskTopic = MQTT_SD_CARD_TASK_TOPIC;
String mqttSensorTaskTopic = MQTT_SENSOR_TASK_TOPIC;

String mqttModemTaskTopic = MQTT_MODEM_TASK_TOPIC;
String mqttDmrTaskTopic = MQTT_DMR_TASK_TOPIC;
String mqttDstarTaskTopic = MQTT_DSTAR_TASK_TOPIC;
String mqttYsfTaskTopic = MQTT_YSF_TASK_TOPIC;
String mqttP25TaskTopic = MQTT_P25_TASK_TOPIC;
String mqttNxdnTaskTopic = MQTT_NXDN_TASK_TOPIC;
String mqttPocsagTaskTopic = MQTT_POCSAG_TASK_TOPIC;
String mqttDapnetTaskTopic = MQTT_DAPNET_TASK_TOPIC;

String mqttSubscribeTopic = MQTT_SUBSCRIBE_TOPIC;
uint16_t mqttSendHardwareInfo = MQTT_SEND_HARDWARE_INFO;
bool mqttHardwareInfoLog = MQTT_HARDWARE_INFO_LOG;
String mqttCommandToken = MQTT_CMD_TOKEN;

// Telegram Settings (loaded from NVS or defaults from config.h)
bool telegramEnabled = TELEGRAM_ENABLED;
String telegramBotToken = TELEGRAM_BOT_TOKEN;
String telegramChatId = TELEGRAM_CHAT_ID;
bool telegramRicForwardEnabled = TELEGRAM_RIC_FORWARD_ENABLED;
String telegramRicList = TELEGRAM_RIC_LIST;
uint16_t telegramRicCooldown = TELEGRAM_RIC_COOLDOWN;

// WireGuard VPN Settings (loaded from NVS or defaults from config.h)
bool wgEnabled = WG_ENABLED;
String wgLocalIp = WG_LOCAL_IP;
String wgPrivateKey = WG_PRIVATE_KEY;
String wgPublicKey = WG_PUBLIC_KEY;
String wgEndpoint = WG_ENDPOINT;
uint16_t wgEndpointPort = WG_ENDPOINT_PORT;
String wgDns = WG_DNS;
String wgAllowedIps = WG_ALLOWED_IPS;
String mqttWgTaskTopic = MQTT_WG_TASK_TOPIC;

// Web Server Settings (loaded from NVS or defaults from config.h)
bool webEnabled = WEB_ENABLED;
uint16_t webServerPort = WEB_SERVER_PORT;
String webUsername = WEB_USERNAME;
String webPassword = WEB_PASSWORD;

// Hardware Settings (LED, Button, OLED) - loaded from NVS or config.h
int ledPin = LED_PIN;
int buttonPin = BUTTON_PIN;

// OLED Settings (loaded from NVS or config.h)
bool oledEnabled = OLED_ENABLED;
int i2cSdaPin = I2C_SDA_PIN;
int i2cSclPin = I2C_SCL_PIN;
int oledI2cAddress = OLED_I2C_ADDRESS;
int oledWidth = OLED_WIDTH;
int oledHeight = OLED_HEIGHT;

// SD Card Settings (loaded from NVS or config.h)
bool sdcardEnabled = SDCARD_ENABLED;
int spiMisoPin = SPI_MISO_PIN;
int spiMosiPin = SPI_MOSI_PIN;
int spiSclkPin = SPI_SCLK_PIN;
int sdCsPin = SD_CS_PIN;

// Ethernet SPI Settings (loaded from NVS or config.h)
int ethMisoPin = ETH_MISO_PIN;
int ethMosiPin = ETH_MOSI_PIN;
int ethSclkPin = ETH_SCLK_PIN;
int ethCsPin = ETH_CS_PIN;
int ethIntPin = ETH_INT_PIN;
int ethRstPin = ETH_RST_PIN;
int ethAddr = ETH_ADDR;
int ethConnectTimeout = ETH_CONNECT_TIMEOUT;

// MMDVM Serial Pin Settings (loaded from NVS or config.h)
int mmdvmRxPin = MMDVM_RX_PIN;
int mmdvmTxPin = MMDVM_TX_PIN;
int mmdvmBootPin = MMDVM_BOOT_PIN;
int mmdvmResetPin = MMDVM_RESET_PIN;
int mmdvmWakeupPin = MMDVM_WAKEUP_PIN;
int mmdvmBaudrate = MMDVM_SERIAL_BAUD;

// MMDVM RF Calibration Settings (loaded from NVS or config.h)
int mmdvmTxDelay = MMDVM_TX_DELAY;
int mmdvmRxLevel = MMDVM_RX_LEVEL;
int mmdvmTxLevel = MMDVM_TX_LEVEL;

// ArduinoOTA Settings (loaded from NVS or config.h)
bool arduinoOtaEnabled = ARDUINO_OTA_ENABLED;
String arduinoOtaPassword = ARDUINO_OTA_PASSWORD;
int arduinoOtaPort = ARDUINO_OTA_PORT;

// Helper: get string from NVS, but treat empty strings as "not set" and return the default
String getStringNonEmpty(const char *key, const char *defaultVal)
{
  String val = preferences.getString(key, defaultVal);
  return val.isEmpty() ? String(defaultVal) : val;
}

/*
 * Load settings from NVS, or use defaults from config.h if not present
 */
void loadSettings()
{
  preferences.begin("mmdvm", false); // Read-write mode

  // Check if settings have been saved before
  bool settingsExist = preferences.getBool("initialized", false);

  if (settingsExist)
  {
    // Load from NVS
    modeDmrEnabled = preferences.getBool("mode_dmr", DEFAULT_MODE_DMR);
    modeDstarEnabled = preferences.getBool("mode_dstar", DEFAULT_MODE_DSTAR);
    modeYsfEnabled = preferences.getBool("mode_ysf", DEFAULT_MODE_YSF);
    modeP25Enabled = preferences.getBool("mode_p25", DEFAULT_MODE_P25);
    modeNxdnEnabled = preferences.getBool("mode_nxdn", DEFAULT_MODE_NXDN);
    modePocsagEnabled = preferences.getBool("mode_pocsag", DEFAULT_MODE_POCSAG);
    dapnetEnabled     = preferences.getBool("dapnet_en",   DAPNET_ENABLED);
    cwidEnabled     = preferences.getBool("cwid_en",     CWID_ENABLED);
    cwidIntervalMin = preferences.getUChar("cwid_int",   CWID_INTERVAL_MIN);
    pocsagFrequency = preferences.getUInt("pocsag_freq", POCSAG_FREQUENCY);
    dapnetServer    = getStringNonEmpty("dapnet_server", DAPNET_SERVER);
    dapnetPort      = preferences.getUShort("dapnet_port", DAPNET_PORT);
    dapnetNodeCs    = preferences.getString("dapnet_cs", DAPNET_NODE_CS);
    dapnetAuthKey   = preferences.getString("dapnet_key", DAPNET_AUTH_KEY);
    dapnetRic       = preferences.getUInt("dapnet_ric", 0);
    pocsagWhitelist = preferences.getString("pocsag_wlist", POCSAG_WHITELIST);
    pocsagBlacklist = preferences.getString("pocsag_blist", POCSAG_BLACKLIST);
    hampagerUser     = preferences.getString("hp_user", HAMPAGER_USER);
    hampagerPassword = preferences.getString("hp_pass", HAMPAGER_PASSWORD);
    hampagerTxGroup  = preferences.getString("hp_txg",  HAMPAGER_TXG);
    userCallsign = getStringNonEmpty("callsign", DMR_CALLSIGN);
    userDmrId = preferences.getUInt("dmr_id", DMR_ID);
    userDmrSsid = preferences.getUChar("dmr_ssid", DMR_SSID);
    dmrRxFreq = preferences.getUInt("dmr_rx_freq", DMR_RX_FREQ);
    dmrTxFreq = preferences.getUInt("dmr_tx_freq", DMR_TX_FREQ);
    dmrColorCode = preferences.getUChar("dmr_color_code", DMR_COLOR_CODE);
    dmrRfPower = preferences.getUChar("dmr_rf_power", DMR_RF_POWER);

    // DMR Server Settings
    dmrServer = getStringNonEmpty("dmr_server", DMR_SERVER);
    dmrPort = preferences.getUShort("dmr_port", DMR_PORT);
    dmrLocalPort = preferences.getUShort("dmr_lport", DMR_LOCAL_PORT);
    dmrPassword = getStringNonEmpty("dmr_pass", DMR_PASSWORD);
    dmrHistorySize = preferences.getUShort("dmr_hist_size", DMR_HISTORY_SIZE);
    dmrActivityTimeout = preferences.getUShort("dmr_act_tout", DMR_ACTIVITY_TIMEOUT);
    dmrUserCacheSize = preferences.getUShort("dmr_usr_cache", DMR_USER_CACHE_SIZE);
    dmrCallsignCacheSize = preferences.getUShort("dmr_cs_cache", DMR_CALLSIGN_CACHE_SIZE);
    dmrApiTimeout = preferences.getUShort("dmr_api_tout", DMR_API_TIMEOUT);

    // Hotspot Settings
    hotspotCallsign = getStringNonEmpty("hs_callsign", HOTSPOT_CALLSIGN);
    hotspotSuffix = getStringNonEmpty("hs_suffix", HOTSPOT_SUFFIX);
    hotspotLatitude = getStringNonEmpty("hs_latitude", HOTSPOT_LATITUDE);
    hotspotLongitude = getStringNonEmpty("hs_longitude", HOTSPOT_LONGITUDE);
    hotspotHeight = preferences.getInt("hs_height", HOTSPOT_HEIGHT);
    hotspotLocation = getStringNonEmpty("hs_location", HOTSPOT_LOCATION);
    hotspotDescription = getStringNonEmpty("hs_desc", HOTSPOT_DESCRIPTION);
    hotspotUrl = getStringNonEmpty("hs_url", HOTSPOT_URL);

    // DMR API Settings
    dmrApiUrl = getStringNonEmpty("dmr_api_url", DMR_API_URL);
    qrzLookupUrl = getStringNonEmpty("qrz_lookup_url", QRZ_LOOKUP_URL);

    // WiFi Station Settings (6 slots)
    wifiSlotLabel[0] = getStringNonEmpty("wifi_s0_lbl", WIFI_SLOT_LABEL);
    wifiSlotSsid[0] = getStringNonEmpty("wifi_ssid", WIFI_SSID);
    wifiSlotPass[0] = getStringNonEmpty("wifi_pass", WIFI_PASSWORD);
    wifiSlotLabel[1] = getStringNonEmpty("wifi_s1_lbl", WIFI_SLOT1_LABEL);
    wifiSlotSsid[1] = getStringNonEmpty("wifi_ssid1", WIFI_SSID1);
    wifiSlotPass[1] = getStringNonEmpty("wifi_pass1", WIFI_PASSWORD1);
    wifiSlotLabel[2] = getStringNonEmpty("wifi_s2_lbl", WIFI_SLOT2_LABEL);
    wifiSlotSsid[2] = getStringNonEmpty("wifi_ssid2", WIFI_SSID2);
    wifiSlotPass[2] = getStringNonEmpty("wifi_pass2", WIFI_PASSWORD2);
    wifiSlotLabel[3] = getStringNonEmpty("wifi_s3_lbl", WIFI_SLOT3_LABEL);
    wifiSlotSsid[3] = getStringNonEmpty("wifi_ssid3", WIFI_SSID3);
    wifiSlotPass[3] = getStringNonEmpty("wifi_pass3", WIFI_PASSWORD3);
    wifiSlotLabel[4] = getStringNonEmpty("wifi_s4_lbl", WIFI_SLOT4_LABEL);
    wifiSlotSsid[4] = getStringNonEmpty("wifi_ssid4", WIFI_SSID4);
    wifiSlotPass[4] = getStringNonEmpty("wifi_pass4", WIFI_PASSWORD4);
    wifiSlotLabel[5] = getStringNonEmpty("wifi_s5_lbl", WIFI_SLOT5_LABEL);
    wifiSlotSsid[5] = getStringNonEmpty("wifi_ssid5", WIFI_SSID5);
    wifiSlotPass[5] = getStringNonEmpty("wifi_pass5", WIFI_PASSWORD5);

    // WiFi AP Settings
    wifiApSsid = getStringNonEmpty("wifi_ap_ssid", WIFI_AP_SSID);
    wifiApPassword = getStringNonEmpty("wifi_ap_pass", WIFI_AP_PASSWORD);
    wifiApChannel = preferences.getUChar("wifi_ap_ch", WIFI_AP_CHANNEL);
    wifiMaxRetries = preferences.getUChar("wifi_max_ret", WIFI_MAX_RETRIES);

    // Ethernet Settings
    ethEnabled = preferences.getBool("eth_enabled", ETH_ENABLED);
    ethDebug = preferences.getBool("eth_debug", ETH_DEBUG);

    // Telegram Settings
    telegramEnabled           = preferences.getBool("tg_en", TELEGRAM_ENABLED);
    telegramBotToken          = preferences.getString("tg_token", TELEGRAM_BOT_TOKEN);
    telegramChatId            = preferences.getString("tg_chat_id", TELEGRAM_CHAT_ID);
    telegramRicForwardEnabled = preferences.getBool("tg_ric_en", TELEGRAM_RIC_FORWARD_ENABLED);
    telegramRicList           = preferences.getString("tg_ric_list", TELEGRAM_RIC_LIST);
    telegramRicCooldown       = preferences.getUShort("tg_ric_cool", TELEGRAM_RIC_COOLDOWN);

    // WireGuard VPN Settings
    wgEnabled = preferences.getBool("wg_en", WG_ENABLED);
    wgLocalIp = getStringNonEmpty("wg_local_ip", WG_LOCAL_IP);
    wgPrivateKey = preferences.getString("wg_priv_key", WG_PRIVATE_KEY);
    wgPublicKey = preferences.getString("wg_pub_key", WG_PUBLIC_KEY);
    wgEndpoint = preferences.getString("wg_endpoint", WG_ENDPOINT);
    wgEndpointPort = preferences.getUShort("wg_ep_port", WG_ENDPOINT_PORT);
    wgDns = getStringNonEmpty("wg_dns", WG_DNS);
    wgAllowedIps = getStringNonEmpty("wg_allowed_ips", WG_ALLOWED_IPS);

    // DNS Settings
    dnsFallbackEnabled = preferences.getBool("dns_fb_en", DNS_FALLBACK_ENABLED);
    dnsFallbackIp = getStringNonEmpty("dns_fb_ip", DNS_FALLBACK_IP);

    // mDNS Settings
    mdnsEnabled = preferences.getBool("mdns_en", ENABLE_MDNS);
    mdnsHostname = getStringNonEmpty("mdns_host", MDNS_HOSTNAME);

    // NTP Settings
    ntpEnabled = preferences.getBool("ntp_en", NTP_ENABLED);
    ntpServer = getStringNonEmpty("ntp_srv", NTP_SERVER);
    ntpGmtOffsetSec = preferences.getInt("ntp_gmt", NTP_GMT_OFFSET_SEC);
    ntpDaylightOffsetSec = preferences.getInt("ntp_dst", NTP_DAYLIGHT_OFFSET_SEC);
    ntpSyncIntervalMs = preferences.getUInt("ntp_sync", NTP_SYNC_INTERVAL_MS);

    // MQTT Settings
    mqttEnabled = preferences.getBool("mqtt_en", MQTT_ENABLED);
    mqttBroker = getStringNonEmpty("mqtt_broker", MQTT_BROKER);
    mqttPort = preferences.getUShort("mqtt_port", MQTT_PORT);
    mqttUser = getStringNonEmpty("mqtt_user", MQTT_USER);
    mqttPassword = getStringNonEmpty("mqtt_pass", MQTT_PASSWORD);
    mqttStatusTopic = getStringNonEmpty("mqtt_status", MQTT_STATUS_TOPIC);
    mqttLogsTopic = getStringNonEmpty("mqtt_logs", MQTT_LOGS_TOPIC);
    mqttHardwareTopic = getStringNonEmpty("mqtt_hw", MQTT_HARDWARE_TOPIC);
    mqttLoggerTaskTopic = getStringNonEmpty("mq_log_task", MQTT_LOGGER_TASK_TOPIC);
    mqttOledTaskTopic = getStringNonEmpty("mqtt_oled_task", MQTT_OLED_TASK_TOPIC);
    mqttLedTaskTopic = getStringNonEmpty("mqtt_led_task", MQTT_LED_TASK_TOPIC);
    mqttWifiTaskTopic = getStringNonEmpty("mqtt_wifi_task", MQTT_WIFI_TASK_TOPIC);
    mqttEthTaskTopic = getStringNonEmpty("mqtt_eth_task", MQTT_ETH_TASK_TOPIC);
    mqttArduinoOtaTaskTopic = getStringNonEmpty("mq_ota_task", MQTT_ARDUINO_OTA_TASK_TOPIC);
    mqttNtpTaskTopic = getStringNonEmpty("mqtt_ntp_task", MQTT_NTP_TASK_TOPIC);
    mqttMqttClientTaskTopic = getStringNonEmpty("mq_mqttc_task", MQTT_MQTT_CLIENT_TASK_TOPIC);
    mqttWebServerTaskTopic = getStringNonEmpty("mq_web_task", MQTT_WEB_SERVER_TASK_TOPIC);
    mqttSdCardTaskTopic = getStringNonEmpty("mq_sd_task", MQTT_SD_CARD_TASK_TOPIC);
    mqttSensorTaskTopic = getStringNonEmpty("mq_sensor_task", MQTT_SENSOR_TASK_TOPIC);
    mqttWgTaskTopic = getStringNonEmpty("mqtt_wg_task", MQTT_WG_TASK_TOPIC);

    mqttModemTaskTopic = getStringNonEmpty("mqtt_modem_task", MQTT_MODEM_TASK_TOPIC);
    mqttDmrTaskTopic = getStringNonEmpty("mqtt_dmr_task", MQTT_DMR_TASK_TOPIC);
    mqttDstarTaskTopic = getStringNonEmpty("mqtt_dstar_task", MQTT_DSTAR_TASK_TOPIC);
    mqttYsfTaskTopic = getStringNonEmpty("mqtt_ysf_task", MQTT_YSF_TASK_TOPIC);
    mqttP25TaskTopic = getStringNonEmpty("mqtt_p25_task", MQTT_P25_TASK_TOPIC);
    mqttNxdnTaskTopic = getStringNonEmpty("mqtt_nxdn_task", MQTT_NXDN_TASK_TOPIC);
    mqttPocsagTaskTopic = getStringNonEmpty("mq_pocsag_task", MQTT_POCSAG_TASK_TOPIC);
    mqttDapnetTaskTopic = getStringNonEmpty("mq_dapnet_task", MQTT_DAPNET_TASK_TOPIC);

    mqttSubscribeTopic = getStringNonEmpty("mqtt_sub", MQTT_SUBSCRIBE_TOPIC);
    mqttSendHardwareInfo = preferences.getUShort("mqtt_hw_int", MQTT_SEND_HARDWARE_INFO);
    mqttHardwareInfoLog = preferences.getBool("mqtt_hw_log", MQTT_HARDWARE_INFO_LOG);
    mqttCommandToken = preferences.getString("mqtt_cmd_tok", MQTT_CMD_TOKEN);

    // Web Server Settings
    webEnabled = preferences.getBool("web_en", WEB_ENABLED);
    webServerPort = preferences.getUShort("web_port", WEB_SERVER_PORT);
    webUsername = getStringNonEmpty("web_user", WEB_USERNAME);
    webPassword = getStringNonEmpty("web_pass", WEB_PASSWORD);

    // Hardware Settings
    ledPin = preferences.getInt("led_pin", LED_PIN);
    buttonPin = preferences.getInt("button_pin", BUTTON_PIN);
    oledEnabled = preferences.getBool("oled_en", OLED_ENABLED);
    i2cSdaPin = preferences.getInt("i2c_sda", I2C_SDA_PIN);
    i2cSclPin = preferences.getInt("i2c_scl", I2C_SCL_PIN);
    oledI2cAddress = preferences.getInt("oled_addr", OLED_I2C_ADDRESS);
    oledWidth = preferences.getInt("oled_w", OLED_WIDTH);
    oledHeight = preferences.getInt("oled_h", OLED_HEIGHT);

    // SD Card Settings
    sdcardEnabled = preferences.getBool("sdcard_en", SDCARD_ENABLED);
    spiMisoPin = preferences.getInt("spi_miso", SPI_MISO_PIN);
    spiMosiPin = preferences.getInt("spi_mosi", SPI_MOSI_PIN);
    spiSclkPin = preferences.getInt("spi_sclk", SPI_SCLK_PIN);
    sdCsPin = preferences.getInt("sd_cs", SD_CS_PIN);

    // Ethernet SPI Settings
    ethMisoPin = preferences.getInt("eth_miso", ETH_MISO_PIN);
    ethMosiPin = preferences.getInt("eth_mosi", ETH_MOSI_PIN);
    ethSclkPin = preferences.getInt("eth_sclk", ETH_SCLK_PIN);
    ethCsPin = preferences.getInt("eth_cs", ETH_CS_PIN);
    ethIntPin = preferences.getInt("eth_int", ETH_INT_PIN);
    ethRstPin = preferences.getInt("eth_rst", ETH_RST_PIN);
    ethAddr = preferences.getInt("eth_addr", ETH_ADDR);
    ethConnectTimeout = preferences.getInt("eth_cto", ETH_CONNECT_TIMEOUT);

    // MMDVM Serial Pin Settings
    mmdvmRxPin = preferences.getInt("mmdvm_rx", MMDVM_RX_PIN);
    mmdvmTxPin = preferences.getInt("mmdvm_tx", MMDVM_TX_PIN);
    mmdvmBootPin = preferences.getInt("mmdvm_boot", MMDVM_BOOT_PIN);
    mmdvmResetPin = preferences.getInt("mmdvm_rst", MMDVM_RESET_PIN);
    mmdvmWakeupPin = preferences.getInt("mmdvm_wakeup", MMDVM_WAKEUP_PIN);
    mmdvmBaudrate = preferences.getInt("mmdvm_baud", MMDVM_SERIAL_BAUD);

    // MMDVM RF Calibration Settings
    mmdvmTxDelay = preferences.getInt("mmdvm_txdly", MMDVM_TX_DELAY);
    mmdvmRxLevel = preferences.getInt("mmdvm_rxlvl", MMDVM_RX_LEVEL);
    mmdvmTxLevel = preferences.getInt("mmdvm_txlvl", MMDVM_TX_LEVEL);

    // ArduinoOTA Settings
    arduinoOtaEnabled = preferences.getBool("ota_en", ARDUINO_OTA_ENABLED);
    arduinoOtaPassword = getStringNonEmpty("ota_pass", ARDUINO_OTA_PASSWORD);
    arduinoOtaPort = preferences.getInt("ota_port", ARDUINO_OTA_PORT);

    addLogMessage("[Settings] Loaded from NVS");
  }
  else
  {
    // First boot - save defaults to NVS
    preferences.putBool("mode_dmr", DEFAULT_MODE_DMR);
    preferences.putBool("mode_dstar", DEFAULT_MODE_DSTAR);
    preferences.putBool("mode_ysf", DEFAULT_MODE_YSF);
    preferences.putBool("mode_p25", DEFAULT_MODE_P25);
    preferences.putBool("mode_nxdn", DEFAULT_MODE_NXDN);
    preferences.putBool("mode_pocsag", DEFAULT_MODE_POCSAG);
    preferences.putBool("dapnet_en",   DAPNET_ENABLED);
    preferences.putBool("cwid_en",     CWID_ENABLED);
    preferences.putUChar("cwid_int",   CWID_INTERVAL_MIN);
    preferences.putString("callsign", DMR_CALLSIGN);
    preferences.putUInt("dmr_id", DMR_ID);
    preferences.putUChar("dmr_ssid", DMR_SSID);
    preferences.putUInt("dmr_rx_freq", DMR_RX_FREQ);
    preferences.putUInt("dmr_tx_freq", DMR_TX_FREQ);
    preferences.putUChar("dmr_color_code", DMR_COLOR_CODE);
    preferences.putUChar("dmr_rf_power", DMR_RF_POWER);

    // WiFi Station Settings (6 slots)
    preferences.putString("wifi_s0_lbl", WIFI_SLOT_LABEL);
    preferences.putString("wifi_ssid", WIFI_SSID);
    preferences.putString("wifi_pass", WIFI_PASSWORD);
    preferences.putString("wifi_s1_lbl", WIFI_SLOT1_LABEL);
    preferences.putString("wifi_ssid1", WIFI_SSID1);
    preferences.putString("wifi_pass1", WIFI_PASSWORD1);
    preferences.putString("wifi_s2_lbl", WIFI_SLOT2_LABEL);
    preferences.putString("wifi_ssid2", WIFI_SSID2);
    preferences.putString("wifi_pass2", WIFI_PASSWORD2);
    preferences.putString("wifi_s3_lbl", WIFI_SLOT3_LABEL);
    preferences.putString("wifi_ssid3", WIFI_SSID3);
    preferences.putString("wifi_pass3", WIFI_PASSWORD3);
    preferences.putString("wifi_s4_lbl", WIFI_SLOT4_LABEL);
    preferences.putString("wifi_ssid4", WIFI_SSID4);
    preferences.putString("wifi_pass4", WIFI_PASSWORD4);
    preferences.putString("wifi_s5_lbl", WIFI_SLOT5_LABEL);
    preferences.putString("wifi_ssid5", WIFI_SSID5);
    preferences.putString("wifi_pass5", WIFI_PASSWORD5);

    // WiFi AP Settings
    preferences.putString("wifi_ap_ssid", WIFI_AP_SSID);
    preferences.putString("wifi_ap_pass", WIFI_AP_PASSWORD);
    preferences.putUChar("wifi_ap_ch", WIFI_AP_CHANNEL);
    preferences.putUChar("wifi_max_ret", WIFI_MAX_RETRIES);

    // Ethernet Settings
    preferences.putBool("eth_enabled", ETH_ENABLED);
    preferences.putBool("eth_debug", ETH_DEBUG);

    // WireGuard VPN Settings
    preferences.putBool("wg_en", WG_ENABLED);
    preferences.putString("wg_local_ip", WG_LOCAL_IP);
    preferences.putString("wg_priv_key", WG_PRIVATE_KEY);
    preferences.putString("wg_pub_key", WG_PUBLIC_KEY);
    preferences.putString("wg_endpoint", WG_ENDPOINT);
    preferences.putUShort("wg_ep_port", WG_ENDPOINT_PORT);

    // DNS Settings
    preferences.putBool("dns_fb_en", DNS_FALLBACK_ENABLED);
    preferences.putString("dns_fb_ip", DNS_FALLBACK_IP);

    // mDNS Settings
    preferences.putBool("mdns_en", ENABLE_MDNS);
    preferences.putString("mdns_host", MDNS_HOSTNAME);

    // NTP Settings
    preferences.putBool("ntp_en", NTP_ENABLED);
    preferences.putString("ntp_srv", NTP_SERVER);
    preferences.putInt("ntp_gmt", NTP_GMT_OFFSET_SEC);
    preferences.putInt("ntp_dst", NTP_DAYLIGHT_OFFSET_SEC);
    preferences.putUInt("ntp_sync", NTP_SYNC_INTERVAL_MS);

    // MQTT Settings
    preferences.putBool("mqtt_en", MQTT_ENABLED);
    preferences.putString("mqtt_broker", MQTT_BROKER);
    preferences.putUShort("mqtt_port", MQTT_PORT);
    preferences.putString("mqtt_user", MQTT_USER);
    preferences.putString("mqtt_pass", MQTT_PASSWORD);
    preferences.putString("mqtt_status", MQTT_STATUS_TOPIC);
    preferences.putString("mqtt_logs", MQTT_LOGS_TOPIC);
    preferences.putString("mqtt_hw", MQTT_HARDWARE_TOPIC);
    preferences.putString("mq_log_task", MQTT_LOGGER_TASK_TOPIC);
    preferences.putString("mqtt_oled_task", MQTT_OLED_TASK_TOPIC);
    preferences.putString("mqtt_led_task", MQTT_LED_TASK_TOPIC);
    preferences.putString("mqtt_wifi_task", MQTT_WIFI_TASK_TOPIC);
    preferences.putString("mqtt_eth_task", MQTT_ETH_TASK_TOPIC);
    preferences.putString("mq_ota_task", MQTT_ARDUINO_OTA_TASK_TOPIC);
    preferences.putString("mqtt_ntp_task", MQTT_NTP_TASK_TOPIC);
    preferences.putString("mq_mqttc_task", MQTT_MQTT_CLIENT_TASK_TOPIC);
    preferences.putString("mq_web_task", MQTT_WEB_SERVER_TASK_TOPIC);
    preferences.putString("mq_sd_task", MQTT_SD_CARD_TASK_TOPIC);
    preferences.putString("mq_sensor_task", MQTT_SENSOR_TASK_TOPIC);

    preferences.putString("mqtt_modem_task", MQTT_MODEM_TASK_TOPIC);
    preferences.putString("mqtt_dmr_task", MQTT_DMR_TASK_TOPIC);
    preferences.putString("mqtt_dstar_task", MQTT_DSTAR_TASK_TOPIC);
    preferences.putString("mqtt_ysf_task", MQTT_YSF_TASK_TOPIC);
    preferences.putString("mqtt_p25_task", MQTT_P25_TASK_TOPIC);
    preferences.putString("mqtt_nxdn_task", MQTT_NXDN_TASK_TOPIC);
    preferences.putString("mq_pocsag_task", MQTT_POCSAG_TASK_TOPIC);
    preferences.putString("mq_dapnet_task", MQTT_DAPNET_TASK_TOPIC);
    preferences.putString("mqtt_sub", MQTT_SUBSCRIBE_TOPIC);
    preferences.putUShort("mqtt_hw_int", MQTT_SEND_HARDWARE_INFO);
    preferences.putBool("mqtt_hw_log", MQTT_HARDWARE_INFO_LOG);

    // Web Server Settings
    preferences.putUShort("web_port", WEB_SERVER_PORT);
    preferences.putString("web_user", WEB_USERNAME);
    preferences.putString("web_pass", WEB_PASSWORD);

    // MMDVM Serial Pin Settings
    preferences.putInt("mmdvm_rx", MMDVM_RX_PIN);
    preferences.putInt("mmdvm_tx", MMDVM_TX_PIN);
    preferences.putInt("mmdvm_boot", MMDVM_BOOT_PIN);
    preferences.putInt("mmdvm_rst", MMDVM_RESET_PIN);
    preferences.putInt("mmdvm_wakeup", MMDVM_WAKEUP_PIN);
    preferences.putInt("mmdvm_baud", MMDVM_SERIAL_BAUD);

    // Hardware Settings
    preferences.putInt("led_pin", LED_PIN);
    preferences.putInt("button_pin", BUTTON_PIN);
    preferences.putBool("oled_en", OLED_ENABLED);
    preferences.putInt("i2c_sda", I2C_SDA_PIN);
    preferences.putInt("i2c_scl", I2C_SCL_PIN);
    preferences.putInt("oled_addr", OLED_I2C_ADDRESS);
    preferences.putInt("oled_w", OLED_WIDTH);
    preferences.putInt("oled_h", OLED_HEIGHT);

    // SD Card Settings
    preferences.putBool("sdcard_en", SDCARD_ENABLED);
    preferences.putInt("spi_miso", SPI_MISO_PIN);
    preferences.putInt("spi_mosi", SPI_MOSI_PIN);
    preferences.putInt("spi_sclk", SPI_SCLK_PIN);
    preferences.putInt("sd_cs", SD_CS_PIN);

    // Ethernet SPI Settings
    preferences.putInt("eth_miso", ETH_MISO_PIN);
    preferences.putInt("eth_mosi", ETH_MOSI_PIN);
    preferences.putInt("eth_sclk", ETH_SCLK_PIN);
    preferences.putInt("eth_cs", ETH_CS_PIN);
    preferences.putInt("eth_int", ETH_INT_PIN);
    preferences.putInt("eth_rst", ETH_RST_PIN);
    preferences.putInt("eth_addr", ETH_ADDR);
    preferences.putInt("eth_cto", ETH_CONNECT_TIMEOUT);

    // MMDVM RF Calibration Settings
    preferences.putInt("mmdvm_txdly", MMDVM_TX_DELAY);
    preferences.putInt("mmdvm_rxlvl", MMDVM_RX_LEVEL);
    preferences.putInt("mmdvm_txlvl", MMDVM_TX_LEVEL);

    // ArduinoOTA Settings
    preferences.putBool("ota_en", ARDUINO_OTA_ENABLED);
    preferences.putString("ota_pass", ARDUINO_OTA_PASSWORD);
    preferences.putInt("ota_port", ARDUINO_OTA_PORT);

    preferences.putBool("initialized", true);
    addLogMessage("[Settings] First boot - saved defaults to NVS");
  }

  preferences.end();

  // Log current settings
  addLogMessage("[Settings] Callsign: " + userCallsign + "-" + String(userDmrSsid) + " DMR ID: " + String(userDmrId));
  addLogMessage("[Settings] Modes - DMR:" + String(modeDmrEnabled ? "ON" : "OFF") + " DSTAR:" + String(modeDstarEnabled ? "ON" : "OFF") + " YSF:" + String(modeYsfEnabled ? "ON" : "OFF") + " P25:" + String(modeP25Enabled ? "ON" : "OFF") + " NXDN:" + String(modeNxdnEnabled ? "ON" : "OFF") + " POCSAG:" + String(modePocsagEnabled ? "ON" : "OFF"));
}

/*
 * Save settings to NVS
 */
void saveSettings()
{
  preferences.begin("mmdvm", false);
  preferences.putBool("mode_dmr", modeDmrEnabled);
  preferences.putBool("mode_dstar", modeDstarEnabled);
  preferences.putBool("mode_ysf", modeYsfEnabled);
  preferences.putBool("mode_p25", modeP25Enabled);
  preferences.putBool("mode_nxdn", modeNxdnEnabled);
  preferences.putBool("mode_pocsag", modePocsagEnabled);
  preferences.putBool("dapnet_en",   dapnetEnabled);
  preferences.putBool("cwid_en",     cwidEnabled);
  preferences.putUChar("cwid_int",   cwidIntervalMin);
  preferences.putUInt("pocsag_freq",     pocsagFrequency);
  preferences.putString("dapnet_server", dapnetServer);
  preferences.putUShort("dapnet_port",   dapnetPort);
  preferences.putString("dapnet_cs",     dapnetNodeCs);
  preferences.putString("dapnet_key",    dapnetAuthKey);
  preferences.putUInt("dapnet_ric",      dapnetRic);
  preferences.putString("pocsag_wlist",  pocsagWhitelist);
  preferences.putString("pocsag_blist",  pocsagBlacklist);
  preferences.putString("hp_user",       hampagerUser);
  preferences.putString("hp_pass",       hampagerPassword);
  preferences.putString("hp_txg",        hampagerTxGroup);
  preferences.putString("callsign", userCallsign);
  preferences.putUInt("dmr_id", userDmrId);
  preferences.putUChar("dmr_ssid", userDmrSsid);
  preferences.putUInt("dmr_rx_freq", dmrRxFreq);
  preferences.putUInt("dmr_tx_freq", dmrTxFreq);
  preferences.putUChar("dmr_color_code", dmrColorCode);
  preferences.putUChar("dmr_rf_power", dmrRfPower);

  // DMR Server Settings
  preferences.putString("dmr_server", dmrServer);
  preferences.putUShort("dmr_port", dmrPort);
  preferences.putUShort("dmr_lport", dmrLocalPort);
  preferences.putString("dmr_pass", dmrPassword);
  preferences.putUShort("dmr_hist_size", dmrHistorySize);
  preferences.putUShort("dmr_act_tout", dmrActivityTimeout);
  preferences.putUShort("dmr_usr_cache", dmrUserCacheSize);
  preferences.putUShort("dmr_cs_cache", dmrCallsignCacheSize);
  preferences.putUShort("dmr_api_tout", dmrApiTimeout);

  // Hotspot Settings
  preferences.putString("hs_callsign", hotspotCallsign);
  preferences.putString("hs_suffix", hotspotSuffix);
  preferences.putString("hs_latitude", hotspotLatitude);
  preferences.putString("hs_longitude", hotspotLongitude);
  preferences.putInt("hs_height", hotspotHeight);
  preferences.putString("hs_location", hotspotLocation);
  preferences.putString("hs_desc", hotspotDescription);
  preferences.putString("hs_url", hotspotUrl);

  // DMR API Settings
  preferences.putString("dmr_api_url", dmrApiUrl);
  preferences.putString("qrz_lookup_url", qrzLookupUrl);

  // WiFi Station Settings (6 slots)
  preferences.putString("wifi_s0_lbl", wifiSlotLabel[0]);
  preferences.putString("wifi_ssid", wifiSlotSsid[0]);
  preferences.putString("wifi_pass", wifiSlotPass[0]);
  preferences.putString("wifi_s1_lbl", wifiSlotLabel[1]);
  preferences.putString("wifi_ssid1", wifiSlotSsid[1]);
  preferences.putString("wifi_pass1", wifiSlotPass[1]);
  preferences.putString("wifi_s2_lbl", wifiSlotLabel[2]);
  preferences.putString("wifi_ssid2", wifiSlotSsid[2]);
  preferences.putString("wifi_pass2", wifiSlotPass[2]);
  preferences.putString("wifi_s3_lbl", wifiSlotLabel[3]);
  preferences.putString("wifi_ssid3", wifiSlotSsid[3]);
  preferences.putString("wifi_pass3", wifiSlotPass[3]);
  preferences.putString("wifi_s4_lbl", wifiSlotLabel[4]);
  preferences.putString("wifi_ssid4", wifiSlotSsid[4]);
  preferences.putString("wifi_pass4", wifiSlotPass[4]);
  preferences.putString("wifi_s5_lbl", wifiSlotLabel[5]);
  preferences.putString("wifi_ssid5", wifiSlotSsid[5]);
  preferences.putString("wifi_pass5", wifiSlotPass[5]);

  // WiFi AP Settings
  preferences.putString("wifi_ap_ssid", wifiApSsid);
  preferences.putString("wifi_ap_pass", wifiApPassword);
  preferences.putUChar("wifi_ap_ch", wifiApChannel);
  preferences.putUChar("wifi_max_ret", wifiMaxRetries);

  // Ethernet Settings
  preferences.putBool("eth_enabled", ethEnabled);
  preferences.putBool("eth_debug", ethDebug);

  // Telegram Settings
  preferences.putBool("tg_en",        telegramEnabled);
  preferences.putString("tg_token",   telegramBotToken);
  preferences.putString("tg_chat_id", telegramChatId);
  preferences.putBool("tg_ric_en",    telegramRicForwardEnabled);
  preferences.putString("tg_ric_list",telegramRicList);
  preferences.putUShort("tg_ric_cool",telegramRicCooldown);

  // WireGuard VPN Settings
  preferences.putBool("wg_en", wgEnabled);
  preferences.putString("wg_local_ip", wgLocalIp);
  preferences.putString("wg_priv_key", wgPrivateKey);
  preferences.putString("wg_pub_key", wgPublicKey);
  preferences.putString("wg_endpoint", wgEndpoint);
  preferences.putUShort("wg_ep_port", wgEndpointPort);
  preferences.putString("wg_dns", wgDns);
  preferences.putString("wg_allowed_ips", wgAllowedIps);
  
  // DNS Settings
  preferences.putBool("dns_fb_en", dnsFallbackEnabled);
  preferences.putString("dns_fb_ip", dnsFallbackIp);

  // mDNS Settings
  preferences.putBool("mdns_en", mdnsEnabled);
  preferences.putString("mdns_host", mdnsHostname);

  // NTP Settings
  preferences.putBool("ntp_en", ntpEnabled);
  preferences.putString("ntp_srv", ntpServer);
  preferences.putInt("ntp_gmt", ntpGmtOffsetSec);
  preferences.putInt("ntp_dst", ntpDaylightOffsetSec);
  preferences.putUInt("ntp_sync", ntpSyncIntervalMs);

  // MQTT Settings
  preferences.putBool("mqtt_en", mqttEnabled);
  preferences.putString("mqtt_broker", mqttBroker);
  preferences.putUShort("mqtt_port", mqttPort);
  preferences.putString("mqtt_user", mqttUser);
  preferences.putString("mqtt_pass", mqttPassword);
  preferences.putString("mqtt_status", mqttStatusTopic);
  preferences.putString("mqtt_logs", mqttLogsTopic);
  preferences.putString("mqtt_hw", mqttHardwareTopic);
  preferences.putString("mqtt_sub", mqttSubscribeTopic);
  preferences.putUShort("mqtt_hw_int", mqttSendHardwareInfo);
  preferences.putBool("mqtt_hw_log", mqttHardwareInfoLog);
  preferences.putString("mqtt_cmd_tok", mqttCommandToken);
  preferences.putString("mq_log_task", mqttLoggerTaskTopic);
  preferences.putString("mqtt_oled_task", mqttOledTaskTopic);
  preferences.putString("mqtt_led_task", mqttLedTaskTopic);
  preferences.putString("mqtt_wifi_task", mqttWifiTaskTopic);
  preferences.putString("mqtt_eth_task", mqttEthTaskTopic);
  preferences.putString("mq_ota_task", mqttArduinoOtaTaskTopic);
  preferences.putString("mqtt_ntp_task", mqttNtpTaskTopic);
  preferences.putString("mq_mqttc_task", mqttMqttClientTaskTopic);
  preferences.putString("mq_web_task", mqttWebServerTaskTopic);
  preferences.putString("mq_sd_task", mqttSdCardTaskTopic);
  preferences.putString("mq_sensor_task", mqttSensorTaskTopic);
  preferences.putString("mqtt_wg_task", mqttWgTaskTopic);

  preferences.putString("mqtt_modem_task", mqttModemTaskTopic);
  preferences.putString("mqtt_dmr_task", mqttDmrTaskTopic);
  preferences.putString("mqtt_dstar_task", mqttDstarTaskTopic);
  preferences.putString("mqtt_ysf_task", mqttYsfTaskTopic);
  preferences.putString("mqtt_p25_task", mqttP25TaskTopic);
  preferences.putString("mqtt_nxdn_task", mqttNxdnTaskTopic);
  preferences.putString("mq_pocsag_task", mqttPocsagTaskTopic);
  preferences.putString("mq_dapnet_task", mqttDapnetTaskTopic);

  // Web Server Settings
  preferences.putBool("web_en", webEnabled);
  preferences.putUShort("web_port", webServerPort);
  preferences.putString("web_user", webUsername);
  preferences.putString("web_pass", webPassword);

  // Hardware Settings
  preferences.putInt("led_pin", ledPin);
  preferences.putInt("button_pin", buttonPin);
  preferences.putBool("oled_en", oledEnabled);
  preferences.putInt("i2c_sda", i2cSdaPin);
  preferences.putInt("i2c_scl", i2cSclPin);
  preferences.putInt("oled_addr", oledI2cAddress);
  preferences.putInt("oled_w", oledWidth);
  preferences.putInt("oled_h", oledHeight);

  // SD Card Settings
  preferences.putBool("sdcard_en", sdcardEnabled);
  preferences.putInt("spi_miso", spiMisoPin);
  preferences.putInt("spi_mosi", spiMosiPin);
  preferences.putInt("spi_sclk", spiSclkPin);
  preferences.putInt("sd_cs", sdCsPin);

  // Ethernet SPI Settings
  preferences.putInt("eth_miso", ethMisoPin);
  preferences.putInt("eth_mosi", ethMosiPin);
  preferences.putInt("eth_sclk", ethSclkPin);
  preferences.putInt("eth_cs", ethCsPin);
  preferences.putInt("eth_int", ethIntPin);
  preferences.putInt("eth_rst", ethRstPin);
  preferences.putInt("eth_addr", ethAddr);
  preferences.putInt("eth_cto", ethConnectTimeout);

  // MMDVM Serial Pin Settings
  preferences.putInt("mmdvm_rx", mmdvmRxPin);
  preferences.putInt("mmdvm_tx", mmdvmTxPin);
  preferences.putInt("mmdvm_boot", mmdvmBootPin);
  preferences.putInt("mmdvm_rst", mmdvmResetPin);
  preferences.putInt("mmdvm_wakeup", mmdvmWakeupPin);
  preferences.putInt("mmdvm_baud", mmdvmBaudrate);

  // MMDVM RF Calibration Settings
  preferences.putInt("mmdvm_txdly", mmdvmTxDelay);
  preferences.putInt("mmdvm_rxlvl", mmdvmRxLevel);
  preferences.putInt("mmdvm_txlvl", mmdvmTxLevel);

  // ArduinoOTA Settings
  preferences.putBool("ota_en", arduinoOtaEnabled);
  preferences.putString("ota_pass", arduinoOtaPassword);
  preferences.putInt("ota_port", arduinoOtaPort);

  preferences.putBool("initialized", true);
  preferences.end();
  addLogMessage("[Settings] Saved to NVS");
}

/*
 * Arduino Setup - Initialize all system modules
 */
void setup()
{
  Serial.begin(115200);
  delay(1000);

  // Initialize logger first to capture all messages
  initLogger();

  addLogMessage("");
  addLogMessage("==========================================");
  addLogMessage("ESP32 MMDVM Web Server - Modular RTOS");
  addLogMessage("==========================================");
  addLogMessage("");

  // Mount internal flash filesystem (LittleFS) for config snapshots.
  // Format on first use if not yet initialised; uses the 'spiffs' partition.
  if (!LittleFS.begin(true)) {
    addLogMessage("[LittleFS] Mount failed — internal flash snapshots unavailable");
  } else {
    addLogMessage("[LittleFS] Mounted (" + String(LittleFS.totalBytes() / 1024) + " KB total, " +
                  String((LittleFS.totalBytes() - LittleFS.usedBytes()) / 1024) + " KB free)");
  }

  // Load settings from NVS (or defaults on first boot)
  loadSettings();
  // Ensure all keys are written to NVS, even if empty/default
  saveSettings();

  addLogMessage("[Setup] Starting system modules...");
  // addLogMessage("");

  // Initialize firmware versions
  initFirmwareVersions();
  addLogMessage("[Setup] Firmware version: " + firmwareVersion);

  // OLED display task
  if (oledEnabled)
  {
    addLogMessage("[Setup] OLED enabled - initializing OLED display");
    initOledTask();
  }

  // LED status indicator task
  addLogMessage("[Setup] Initializing LED task");
  initLedTask();

  // Initialize WiFi task
  addLogMessage("[Setup] Initializing WiFi task");
  initWiFiTask();

  // Optionally initialize Ethernet (can run alongside WiFi)
  if (ethEnabled)
  {
    addLogMessage("[Setup] Ethernet enabled - will connect in parallel with WiFi");
    initEthTask();
  }

  // Initialize WireGuard VPN (if enabled)
  if (wgEnabled)
  {
    addLogMessage("[Setup] WireGuard enabled - initializing WireGuard VPN");
    initWireguardTask();
  }

  // Initialize Telegram task only if enabled at boot (can also be started on-demand via web UI)
  if (telegramEnabled)
  {
    addLogMessage("[Setup] Initializing Telegram task");
    initTelegramTask();
  }

  // Initialize MQTT client (if enabled)
  if (mqttEnabled)
  {
    addLogMessage("[Setup] MQTT enabled - initializing MQTT client");
    initMqttTask();
  }
  // Initialize ArduinoOTA task (if enabled)
  if (arduinoOtaEnabled)
  {
    addLogMessage("[Setup] ArduinoOTA enabled - initializing ArduinoOTA task");
    initArduinoOtaTask();
  }
  // Initialize NTP time synchronization
  if (ntpEnabled)
  {
    addLogMessage("[Setup] Initializing NTP time sync");
    initNtpTask();
  }

  // Web server task
  addLogMessage("[Setup] Initializing Web Server task");
  initWebServerTask();

  // Initialize SD Card (if enabled)
  if (sdcardEnabled)
  {
    addLogMessage("[Setup] SD Card enabled - initializing SD Card");
    initSdCardTask();
  }

  // Sensor monitoring task
  // addLogMessage("[Setup] Initializing Sensor task");
  // initSensorTask();

  // Initialize MMDVM Modem BEFORE protocol tasks
  // addLogMessage("");
  addLogMessage("[Setup] Initializing MMDVM Modem...");
  initModemTask();

  // Wait for modem to be ready (max 10 seconds)
  addLogMessage("[Setup] Waiting for modem initialization...");
  int modemWaitCount = 0;
  while (!mmdvmReady && modemWaitCount < 100)
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    modemWaitCount++;
  }

  if (mmdvmReady)
  {
    addLogMessage("[Setup] MMDVM Modem ready!");
  }
  else
  {
    addLogMessage("[Setup] WARNING: Modem not ready, continuing anyway...");
  }

  // Initialize MMDVM protocol tasks (based on NVS settings)
  // addLogMessage("");
  addLogMessage("[Setup] Initializing MMDVM Protocol Tasks...");

  if (modeDmrEnabled)
  {
    initDmrTask();
    initDmrLookupTask();
  }

  if (modeDstarEnabled)
  {
    initDstarTask();
  }

  if (modeYsfEnabled)
  {
    initYsfTask();
  }

  if (modeP25Enabled)
  {
    initP25Task();
  }

  if (modeNxdnEnabled)
  {
    initNxdnTask();
  }

  // POCSAG TX task — only if POCSAG mode is enabled
  if (modePocsagEnabled) {
    initPocsagTask();

    // DAPNET network client — only if DAPNET is also enabled
    if (dapnetEnabled) {
      initDapnetTask();
    }
  }

  addLogMessage("[Setup] All system modules initialized!");
  addLogMessage("[Setup] RTOS scheduler managing all tasks");
  addLogMessage("[Setup] --- System is up and running ---");
}

/*
 * Arduino Loop - EMPTY!
 *
 * This is the beauty of RTOS:
 * No complex loop with millis() checks, no state machines, no blocking delays!
 * The RTOS scheduler handles everything automatically.
 *
 * In a plain loop, this would be filled with:
 * - if (millis() - lastWiFiCheck > 5000) { checkWiFi(); }
 * - if (millis() - lastLedToggle > 1000) { toggleLED(); }
 * - server.handleClient();
 * - etc...
 *
 * And if WiFi reconnection blocks, EVERYTHING stops!
 */
void loop()
{
  // Nothing here! RTOS tasks do all the work.
  // This loop is actually part of the "idle" task.
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
