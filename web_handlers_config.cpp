/*
 * web_handlers_config.cpp - Configuration Export & Import Routes
 *
 * Extracted from web_handlers_admin.cpp.
 * All routes registered here use the global `server` object (extern WebServer server).
 *
 * Routes:
 *   /api/export-config  - dump all settings as key=value text (attachment download)
 *   /api/import-config  - parse key=value text and apply settings
 */

#include "system/web_handlers_config.h"
#include "system/system_webserver.h"   // extern WebServer server
#include "system/system_logger.h"      // addLogMessage()
#include "include/config.h"            // compile-time defaults
#include <Preferences.h>

// Runtime mode settings (defined in esp32-rtos-mmdvm.ino)
extern bool modeDmrEnabled;
extern bool modeDstarEnabled;
extern bool modeYsfEnabled;
extern bool modeP25Enabled;
extern bool modeNxdnEnabled;
extern bool modePocsagEnabled;
extern bool cwidEnabled;
extern uint8_t cwidIntervalMin;
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
extern String mqttDapnetTaskTopic;
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
extern bool sdcardEnabled;
extern int spiMisoPin;
extern int spiMosiPin;
extern int spiSclkPin;
extern int sdCsPin;
extern bool mqttConnected;
extern unsigned long lastMqttAttempt;
extern void saveSettings();

// ---------------------------------------------------------------------------
// generateConfigString() — serialise all current settings to key=value text.
// Shared by the export-config HTTP route and the snapshot-save handler.
// ---------------------------------------------------------------------------
String generateConfigString()
{
    String config = "# ESP32 MMDVM Configuration Export\n";
    config += "# Generated: " + String(millis() / 1000) + "s uptime\n\n";

    config += "# Mode Settings\n";
    config += "mode_dmr=" + String(modeDmrEnabled) + "\n";
    config += "mode_dstar=" + String(modeDstarEnabled) + "\n";
    config += "mode_ysf=" + String(modeYsfEnabled) + "\n";
    config += "mode_p25=" + String(modeP25Enabled) + "\n";
    config += "mode_nxdn=" + String(modeNxdnEnabled) + "\n";
    config += "mode_pocsag=" + String(modePocsagEnabled) + "\n";
    config += "dapnet_en=" + String(dapnetEnabled) + "\n";

    config += "\n# CW ID Settings\n";
    config += "cwid_en=" + String(cwidEnabled) + "\n";
    config += "cwid_int=" + String(cwidIntervalMin) + "\n";

    config += "\n# DAPNET / POCSAG Network Settings\n";
    config += "pocsag_freq=" + String(pocsagFrequency) + "\n";
    config += "dapnet_server=" + dapnetServer + "\n";
    config += "dapnet_port=" + String(dapnetPort) + "\n";
    config += "dapnet_cs=" + dapnetNodeCs + "\n";
    config += "dapnet_key=" + dapnetAuthKey + "\n";
    config += "dapnet_ric=" + String(dapnetRic) + "\n";
    config += "pocsag_wlist=" + pocsagWhitelist + "\n";
    config += "pocsag_blist=" + pocsagBlacklist + "\n";

    config += "\n# Station Settings\n";
    config += "callsign=" + userCallsign + "\n";
    config += "dmr_id=" + String(userDmrId) + "\n";
    config += "dmr_ssid=" + String(userDmrSsid) + "\n";

    config += "\n# RF Settings\n";
    config += "dmr_rx_freq=" + String(dmrRxFreq) + "\n";
    config += "dmr_tx_freq=" + String(dmrTxFreq) + "\n";
    config += "dmr_color_code=" + String(dmrColorCode) + "\n";
    config += "dmr_rf_power=" + String(dmrRfPower) + "\n";

    config += "\n# DMR Server Settings\n";
    config += "dmr_server=" + dmrServer + "\n";
    config += "dmr_port=" + String(dmrPort) + "\n";
    config += "dmr_lport=" + String(dmrLocalPort) + "\n";
    config += "dmr_pass=" + dmrPassword + "\n";
    config += "dmr_hist_size=" + String(dmrHistorySize) + "\n";
    config += "dmr_act_tout=" + String(dmrActivityTimeout) + "\n";
    config += "dmr_usr_cache=" + String(dmrUserCacheSize) + "\n";
    config += "dmr_cs_cache=" + String(dmrCallsignCacheSize) + "\n";
    config += "dmr_api_tout=" + String(dmrApiTimeout) + "\n";

    config += "\n# Hotspot Settings\n";
    config += "hs_callsign=" + hotspotCallsign + "\n";
    config += "hs_suffix=" + hotspotSuffix + "\n";
    config += "hs_latitude=" + hotspotLatitude + "\n";
    config += "hs_longitude=" + hotspotLongitude + "\n";
    config += "hs_height=" + String(hotspotHeight) + "\n";
    config += "hs_location=" + hotspotLocation + "\n";
    config += "hs_desc=" + hotspotDescription + "\n";
    config += "hs_url=" + hotspotUrl + "\n";

    config += "\n# DMR API Settings\n";
    config += "dmr_api_url=" + dmrApiUrl + "\n";
    config += "qrz_lookup_url=" + qrzLookupUrl + "\n";

    config += "\n# WiFi Station Settings (6 slots)\n";
    config += "wifi_s0_lbl=" + wifiSlotLabel[0] + "\n";
    config += "wifi_ssid=" + wifiSlotSsid[0] + "\n";
    config += "wifi_pass=" + wifiSlotPass[0] + "\n";
    config += "wifi_s1_lbl=" + wifiSlotLabel[1] + "\n";
    config += "wifi_ssid1=" + wifiSlotSsid[1] + "\n";
    config += "wifi_pass1=" + wifiSlotPass[1] + "\n";
    config += "wifi_s2_lbl=" + wifiSlotLabel[2] + "\n";
    config += "wifi_ssid2=" + wifiSlotSsid[2] + "\n";
    config += "wifi_pass2=" + wifiSlotPass[2] + "\n";
    config += "wifi_s3_lbl=" + wifiSlotLabel[3] + "\n";
    config += "wifi_ssid3=" + wifiSlotSsid[3] + "\n";
    config += "wifi_pass3=" + wifiSlotPass[3] + "\n";
    config += "wifi_s4_lbl=" + wifiSlotLabel[4] + "\n";
    config += "wifi_ssid4=" + wifiSlotSsid[4] + "\n";
    config += "wifi_pass4=" + wifiSlotPass[4] + "\n";
    config += "wifi_s5_lbl=" + wifiSlotLabel[5] + "\n";
    config += "wifi_ssid5=" + wifiSlotSsid[5] + "\n";
    config += "wifi_pass5=" + wifiSlotPass[5] + "\n";

    config += "\n# WiFi AP Settings\n";
    config += "wifi_ap_ssid=" + wifiApSsid + "\n";
    config += "wifi_ap_pass=" + wifiApPassword + "\n";
    config += "wifi_ap_ch=" + String(wifiApChannel) + "\n";
    config += "wifi_max_ret=" + String(wifiMaxRetries) + "\n";

    config += "\n# Ethernet Settings\n";
    config += "eth_enabled=" + String(ethEnabled) + "\n";
    config += "eth_debug=" + String(ethDebug) + "\n";

    config += "\n# WireGuard VPN Settings\n";
    config += "wg_en=" + String(wgEnabled) + "\n";
    config += "wg_local_ip=" + wgLocalIp + "\n";
    config += "wg_priv_key=" + wgPrivateKey + "\n";
    config += "wg_pub_key=" + wgPublicKey + "\n";
    config += "wg_endpoint=" + wgEndpoint + "\n";
    config += "wg_ep_port=" + String(wgEndpointPort) + "\n";
    config += "wg_dns=" + wgDns + "\n";
    config += "wg_allowed_ips=" + wgAllowedIps + "\n";

    config += "\n# DNS Settings\n";
    config += "dns_fb_en=" + String(dnsFallbackEnabled) + "\n";
    config += "dns_fb_ip=" + dnsFallbackIp + "\n";

    config += "\n# mDNS Settings\n";
    config += "mdns_en=" + String(mdnsEnabled) + "\n";
    config += "mdns_host=" + mdnsHostname + "\n";

    config += "\n# NTP Settings\n";
    config += "ntp_en=" + String(ntpEnabled) + "\n";
    config += "ntp_srv=" + ntpServer + "\n";
    config += "ntp_gmt=" + String(ntpGmtOffsetSec) + "\n";
    config += "ntp_dst=" + String(ntpDaylightOffsetSec) + "\n";
    config += "ntp_sync=" + String(ntpSyncIntervalMs) + "\n";

    config += "\n# MQTT Settings\n";
    config += "mqtt_en=" + String(mqttEnabled) + "\n";
    config += "mqtt_broker=" + mqttBroker + "\n";
    config += "mqtt_port=" + String(mqttPort) + "\n";
    config += "mqtt_user=" + mqttUser + "\n";
    config += "mqtt_pass=" + mqttPassword + "\n";
    config += "mqtt_status=" + mqttStatusTopic + "\n";
    config += "mqtt_logs=" + mqttLogsTopic + "\n";
    config += "mqtt_hw=" + mqttHardwareTopic + "\n";
    config += "mq_log_task=" + mqttLoggerTaskTopic + "\n";
    config += "mqtt_oled_task=" + mqttOledTaskTopic + "\n";
    config += "mqtt_led_task=" + mqttLedTaskTopic + "\n";
    config += "mqtt_wifi_task=" + mqttWifiTaskTopic + "\n";
    config += "mqtt_eth_task=" + mqttEthTaskTopic + "\n";
    config += "mq_ota_task=" + mqttArduinoOtaTaskTopic + "\n";
    config += "mqtt_ntp_task=" + mqttNtpTaskTopic + "\n";
    config += "mq_mqttc_task=" + mqttMqttClientTaskTopic + "\n";
    config += "mq_web_task=" + mqttWebServerTaskTopic + "\n";
    config += "mq_sd_task=" + mqttSdCardTaskTopic + "\n";
    config += "mq_sensor_task=" + mqttSensorTaskTopic + "\n";
    config += "mqtt_wg_task=" + mqttWgTaskTopic + "\n";

    config += "mqtt_modem_task=" + mqttModemTaskTopic + "\n";
    config += "mqtt_dmr_task=" + mqttDmrTaskTopic + "\n";
    config += "mqtt_dstar_task=" + mqttDstarTaskTopic + "\n";
    config += "mqtt_ysf_task=" + mqttYsfTaskTopic + "\n";
    config += "mqtt_p25_task=" + mqttP25TaskTopic + "\n";
    config += "mqtt_nxdn_task=" + mqttNxdnTaskTopic + "\n";
    config += "mq_pocsag_task=" + mqttPocsagTaskTopic + "\n";
    config += "mq_dapnet_task=" + mqttDapnetTaskTopic + "\n";

    config += "mqtt_sub=" + mqttSubscribeTopic + "\n";
    config += "mqtt_hw_int=" + String(mqttSendHardwareInfo) + "\n";
    config += "mqtt_hw_log=" + String(mqttHardwareInfoLog) + "\n";

    config += "\n# Web Server Settings\n";
    config += "web_en=" + String(webEnabled) + "\n";
    config += "web_user=" + webUsername + "\n";
    config += "web_pass=" + webPassword + "\n";
    config += "web_port=" + String(webServerPort) + "\n";

    config += "\n# Hardware Settings\n";
    config += "led_pin=" + String(ledPin) + "\n";
    config += "button_pin=" + String(buttonPin) + "\n";

    config += "\n# OLED Settings\n";
    config += "oled_en=" + String(oledEnabled) + "\n";
    config += "i2c_sda=" + String(i2cSdaPin) + "\n";
    config += "i2c_scl=" + String(i2cSclPin) + "\n";
    config += "oled_addr=" + String(oledI2cAddress) + "\n";
    config += "oled_w=" + String(oledWidth) + "\n";
    config += "oled_h=" + String(oledHeight) + "\n";

    config += "\n# SD Card Settings\n";
    config += "sdcard_en=" + String(sdcardEnabled) + "\n";
    config += "spi_miso=" + String(spiMisoPin) + "\n";
    config += "spi_mosi=" + String(spiMosiPin) + "\n";
    config += "spi_sclk=" + String(spiSclkPin) + "\n";
    config += "sd_cs=" + String(sdCsPin) + "\n";

    {
      extern int ethMisoPin, ethMosiPin, ethSclkPin, ethCsPin, ethIntPin, ethRstPin, ethAddr, ethConnectTimeout;
      config += "\n# Ethernet SPI Settings\n";
      config += "eth_miso=" + String(ethMisoPin) + "\n";
      config += "eth_mosi=" + String(ethMosiPin) + "\n";
      config += "eth_sclk=" + String(ethSclkPin) + "\n";
      config += "eth_cs=" + String(ethCsPin) + "\n";
      config += "eth_int=" + String(ethIntPin) + "\n";
      config += "eth_rst=" + String(ethRstPin) + "\n";
      config += "eth_addr=" + String(ethAddr) + "\n";
      config += "eth_cto=" + String(ethConnectTimeout) + "\n";
    }

    {
      extern int mmdvmRxPin, mmdvmTxPin, mmdvmBootPin, mmdvmResetPin, mmdvmWakeupPin, mmdvmBaudrate;
      extern int mmdvmTxDelay, mmdvmRxLevel, mmdvmTxLevel;
      config += "\n# MMDVM Settings\n";
      config += "mmdvm_rx=" + String(mmdvmRxPin) + "\n";
      config += "mmdvm_tx=" + String(mmdvmTxPin) + "\n";
      config += "mmdvm_boot=" + String(mmdvmBootPin) + "\n";
      config += "mmdvm_rst=" + String(mmdvmResetPin) + "\n";
      config += "mmdvm_wakeup=" + String(mmdvmWakeupPin) + "\n";
      config += "mmdvm_baud=" + String(mmdvmBaudrate) + "\n";
      config += "mmdvm_txdly=" + String(mmdvmTxDelay) + "\n";
      config += "mmdvm_rxlvl=" + String(mmdvmRxLevel) + "\n";
      config += "mmdvm_txlvl=" + String(mmdvmTxLevel) + "\n";
    }

    {
      extern bool arduinoOtaEnabled;
      extern String arduinoOtaPassword;
      extern int arduinoOtaPort;
      config += "\n# ArduinoOTA Settings\n";
      config += "ota_en=" + String(arduinoOtaEnabled) + "\n";
      config += "ota_pass=" + arduinoOtaPassword + "\n";
      config += "ota_port=" + String(arduinoOtaPort) + "\n";
    }

    return config;
}

// ---------------------------------------------------------------------------
// applyConfigString() — parse key=value text and write values to globals.
// Returns the number of recognised settings applied.
// Shared by the import-config HTTP route and the snapshot-load handler.
// ---------------------------------------------------------------------------
int applyConfigString(const String& body)
{
    int imported = 0;
    int lineStart = 0;

    while (lineStart < (int)body.length()) {
      int lineEnd = body.indexOf('\n', lineStart);
      if (lineEnd == -1) lineEnd = body.length();

      String line = body.substring(lineStart, lineEnd);
      line.trim();
      lineStart = lineEnd + 1;

      // Skip comments and empty lines
      if (line.length() == 0 || line.startsWith("#")) continue;

      int eq = line.indexOf('=');
      if (eq <= 0) continue;

      String key = line.substring(0, eq);
      String val = line.substring(eq + 1);
      key.trim();
      val.trim();

      // Apply settings based on key
      if (key == "wg_dns") {
        wgDns = val;
        imported++;
      } else if (key == "wg_allowed_ips") {
        wgAllowedIps = val;
        imported++;
      } else if (key == "mode_dmr") {
        modeDmrEnabled = (val == "1");
        imported++;
      } else if (key == "mode_dstar") {
        modeDstarEnabled = (val == "1");
        imported++;
      } else if (key == "mode_ysf") {
        modeYsfEnabled = (val == "1");
        imported++;
      } else if (key == "mode_p25") {
        modeP25Enabled = (val == "1");
        imported++;
      } else if (key == "mode_nxdn") {
        modeNxdnEnabled = (val == "1");
        imported++;
      } else if (key == "mode_pocsag") {
        modePocsagEnabled = (val == "1");
        imported++;
      } else if (key == "dapnet_en") {
        dapnetEnabled = (val == "1");
        imported++;
      } else if (key == "cwid_en") {
        cwidEnabled = (val == "1");
        imported++;
      } else if (key == "cwid_int") {
        cwidIntervalMin = (uint8_t)val.toInt();
        imported++;
      } else if (key == "callsign") {
        userCallsign = val;
        imported++;
      } else if (key == "dmr_id") {
        userDmrId = val.toInt();
        imported++;
      } else if (key == "dmr_ssid") {
        userDmrSsid = val.toInt();
        imported++;
      } else if (key == "dmr_rx_freq") {
        dmrRxFreq = val.toInt();
        imported++;
      } else if (key == "dmr_tx_freq") {
        dmrTxFreq = val.toInt();
        imported++;
      } else if (key == "dmr_color_code") {
        dmrColorCode = val.toInt();
        imported++;
      } else if (key == "dmr_rf_power") {
        dmrRfPower = val.toInt();
        imported++;
      } else if (key == "dmr_server") {
        dmrServer = val;
        imported++;
      } else if (key == "dmr_port") {
        dmrPort = val.toInt();
        imported++;
      } else if (key == "dmr_lport") {
        dmrLocalPort = val.toInt();
        imported++;
      } else if (key == "dmr_pass") {
        dmrPassword = val;
        imported++;
      } else if (key == "dmr_hist_size") {
        dmrHistorySize = val.toInt();
        imported++;
      } else if (key == "dmr_act_tout") {
        dmrActivityTimeout = val.toInt();
        imported++;
      } else if (key == "dmr_usr_cache") {
        dmrUserCacheSize = val.toInt();
        imported++;
      } else if (key == "dmr_cs_cache") {
        dmrCallsignCacheSize = val.toInt();
        imported++;
      } else if (key == "dmr_api_tout") {
        dmrApiTimeout = val.toInt();
        imported++;
      } else if (key == "hs_callsign") {
        hotspotCallsign = val;
        imported++;
      } else if (key == "hs_suffix") {
        hotspotSuffix = val;
        imported++;
      } else if (key == "hs_latitude") {
        hotspotLatitude = val;
        imported++;
      } else if (key == "hs_longitude") {
        hotspotLongitude = val;
        imported++;
      } else if (key == "hs_height") {
        hotspotHeight = val.toInt();
        imported++;
      } else if (key == "hs_location") {
        hotspotLocation = val;
        imported++;
      } else if (key == "hs_desc") {
        hotspotDescription = val;
        imported++;
      } else if (key == "hs_url") {
        hotspotUrl = val;
        imported++;
      } else if (key == "dmr_api_url") {
        dmrApiUrl = val;
        imported++;
      } else if (key == "qrz_lookup_url") {
        qrzLookupUrl = val;
        imported++;
      } else if (key == "wifi_s0_lbl") {
        wifiSlotLabel[0] = val;
        imported++;
      } else if (key == "wifi_ssid") {
        wifiSlotSsid[0] = val;
        imported++;
      } else if (key == "wifi_pass") {
        wifiSlotPass[0] = val;
        imported++;
      } else if (key == "wifi_s1_lbl") {
        wifiSlotLabel[1] = val;
        imported++;
      } else if (key == "wifi_ssid1") {
        wifiSlotSsid[1] = val;
        imported++;
      } else if (key == "wifi_pass1") {
        wifiSlotPass[1] = val;
        imported++;
      } else if (key == "wifi_s2_lbl") {
        wifiSlotLabel[2] = val;
        imported++;
      } else if (key == "wifi_ssid2") {
        wifiSlotSsid[2] = val;
        imported++;
      } else if (key == "wifi_pass2") {
        wifiSlotPass[2] = val;
        imported++;
      } else if (key == "wifi_s3_lbl") {
        wifiSlotLabel[3] = val;
        imported++;
      } else if (key == "wifi_ssid3") {
        wifiSlotSsid[3] = val;
        imported++;
      } else if (key == "wifi_pass3") {
        wifiSlotPass[3] = val;
        imported++;
      } else if (key == "wifi_s4_lbl") {
        wifiSlotLabel[4] = val;
        imported++;
      } else if (key == "wifi_ssid4") {
        wifiSlotSsid[4] = val;
        imported++;
      } else if (key == "wifi_pass4") {
        wifiSlotPass[4] = val;
        imported++;
      } else if (key == "wifi_s5_lbl") {
        wifiSlotLabel[5] = val;
        imported++;
      } else if (key == "wifi_ssid5") {
        wifiSlotSsid[5] = val;
        imported++;
      } else if (key == "wifi_pass5") {
        wifiSlotPass[5] = val;
        imported++;
      } else if (key == "wifi_ap_ssid") {
        wifiApSsid = val;
        imported++;
      } else if (key == "wifi_ap_pass") {
        wifiApPassword = val;
        imported++;
      } else if (key == "wifi_ap_ch") {
        wifiApChannel = val.toInt();
        imported++;
      } else if (key == "wifi_max_ret") {
        wifiMaxRetries = val.toInt();
        imported++;
      } else if (key == "eth_enabled") {
        ethEnabled = (val == "1");
        imported++;
      } else if (key == "eth_debug") {
        ethDebug = (val == "1");
        imported++;
      } else if (key == "wg_en") {
        wgEnabled = (val == "1");
        imported++;
      } else if (key == "wg_local_ip") {
        wgLocalIp = val;
        imported++;
      } else if (key == "wg_priv_key") {
        wgPrivateKey = val;
        imported++;
      } else if (key == "wg_pub_key") {
        wgPublicKey = val;
        imported++;
      } else if (key == "wg_endpoint") {
        wgEndpoint = val;
        imported++;
      } else if (key == "wg_ep_port") {
        wgEndpointPort = val.toInt();
        imported++;
      } else if (key == "dns_fb_en") {
        dnsFallbackEnabled = (val == "1");
        imported++;
      } else if (key == "dns_fb_ip") {
        dnsFallbackIp = val;
        imported++;
      } else if (key == "mdns_en") {
        mdnsEnabled = (val == "1");
        imported++;
      } else if (key == "mdns_host") {
        mdnsHostname = val;
        imported++;
      } else if (key == "ntp_en") {
        ntpEnabled = (val == "1");
        imported++;
      } else if (key == "ntp_srv") {
        ntpServer = val;
        imported++;
      } else if (key == "ntp_gmt") {
        ntpGmtOffsetSec = val.toInt();
        imported++;
      } else if (key == "ntp_dst") {
        ntpDaylightOffsetSec = val.toInt();
        imported++;
      } else if (key == "ntp_sync") {
        ntpSyncIntervalMs = val.toInt();
        imported++;
      } else if (key == "mqtt_en") {
        mqttEnabled = (val == "1");
        imported++;
      } else if (key == "mqtt_broker") {
        mqttBroker = val;
        imported++;
      } else if (key == "mqtt_port") {
        mqttPort = val.toInt();
        imported++;
      } else if (key == "mqtt_user") {
        mqttUser = val;
        imported++;
      } else if (key == "mqtt_pass") {
        mqttPassword = val;
        imported++;
      } else if (key == "mqtt_status") {
        mqttStatusTopic = val;
        imported++;
      } else if (key == "mqtt_logs") {
        mqttLogsTopic = val;
        imported++;
      } else if (key == "mqtt_hw") {
        mqttHardwareTopic = val;
        imported++;

      } else if (key == "mq_log_task") {
        mqttLoggerTaskTopic = val;
        imported++;
      } else if (key == "mqtt_oled_task") {
        mqttOledTaskTopic = val;
        imported++;
      } else if (key == "mqtt_led_task") {
        mqttLedTaskTopic = val;
        imported++;
      } else if (key == "mqtt_wifi_task") {
        mqttWifiTaskTopic = val;
        imported++;
      } else if (key == "mqtt_eth_task") {
        mqttEthTaskTopic = val;
        imported++;
      } else if (key == "mq_ota_task") {
        mqttArduinoOtaTaskTopic = val;
        imported++;
      } else if (key == "mqtt_ntp_task") {
        mqttNtpTaskTopic = val;
        imported++;
      } else if (key == "mq_mqttc_task") {
        mqttMqttClientTaskTopic = val;
        imported++;
      } else if (key == "mq_web_task") {
        mqttWebServerTaskTopic = val;
        imported++;
      } else if (key == "mq_sd_task") {
        mqttSdCardTaskTopic = val;
        imported++;
      } else if (key == "mq_sensor_task") {
        mqttSensorTaskTopic = val;
        imported++;
      } else if (key == "mqtt_wg_task") {
        mqttWgTaskTopic = val;
        imported++;
      } else if (key == "mqtt_modem_task") {
        mqttModemTaskTopic = val;
        imported++;
      } else if (key == "mqtt_dmr_task") {
        mqttDmrTaskTopic = val;
        imported++;
      } else if (key == "mqtt_dstar_task") {
        mqttDstarTaskTopic = val;
        imported++;
      } else if (key == "mqtt_ysf_task") {
        mqttYsfTaskTopic = val;
        imported++;
      } else if (key == "mqtt_p25_task") {
        mqttP25TaskTopic = val;
        imported++;
      } else if (key == "mqtt_nxdn_task") {
        mqttNxdnTaskTopic = val;
        imported++;
      } else if (key == "mq_pocsag_task") {
        mqttPocsagTaskTopic = val;
        imported++;
      } else if (key == "mq_dapnet_task") {
        mqttDapnetTaskTopic = val;
        imported++;
      } else if (key == "pocsag_freq") {
        pocsagFrequency = val.toInt();
        imported++;
      } else if (key == "dapnet_server") {
        dapnetServer = val;
        imported++;
      } else if (key == "dapnet_port") {
        dapnetPort = val.toInt();
        imported++;
      } else if (key == "dapnet_cs") {
        dapnetNodeCs = val;
        imported++;
      } else if (key == "dapnet_key") {
        dapnetAuthKey = val;
        imported++;
      } else if (key == "dapnet_ric") {
        dapnetRic = val.toInt();
        imported++;
      } else if (key == "pocsag_wlist") {
        pocsagWhitelist = val;
        imported++;
      } else if (key == "pocsag_blist") {
        pocsagBlacklist = val;
        imported++;

      } else if (key == "mqtt_sub") {
        mqttSubscribeTopic = val;
        imported++;
      } else if (key == "mqtt_hw_int") {
        mqttSendHardwareInfo = val.toInt();
        imported++;
      } else if (key == "mqtt_hw_log") {
        mqttHardwareInfoLog = (val == "1");
        imported++;
      } else if (key == "web_port") {
        webServerPort = val.toInt();
        imported++;
      } else if (key == "web_user") {
        webUsername = val;
        imported++;
      } else if (key == "web_en") {
        webEnabled = (val == "1");
        imported++;
      } else if (key == "web_pass") {
        webPassword = val;
        imported++;
      } else if (key == "led_pin") {
        ledPin = val.toInt();
        imported++;
      } else if (key == "button_pin") {
        buttonPin = val.toInt();
        imported++;
      } else if (key == "oled_en") {
        oledEnabled = (val == "1");
        imported++;
      } else if (key == "i2c_sda") {
        i2cSdaPin = val.toInt();
        imported++;
      } else if (key == "i2c_scl") {
        i2cSclPin = val.toInt();
        imported++;
      } else if (key == "oled_addr") {
        oledI2cAddress = val.toInt();
        imported++;
      } else if (key == "oled_w") {
        oledWidth = val.toInt();
        imported++;
      } else if (key == "oled_h") {
        oledHeight = val.toInt();
        imported++;
      } else if (key == "sdcard_en") {
        sdcardEnabled = (val == "1");
        imported++;
      } else if (key == "spi_miso") {
        spiMisoPin = val.toInt();
        imported++;
      } else if (key == "spi_mosi") {
        spiMosiPin = val.toInt();
        imported++;
      } else if (key == "spi_sclk") {
        spiSclkPin = val.toInt();
        imported++;
      } else if (key == "sd_cs") {
        sdCsPin = val.toInt();
        imported++;
      } else if (key == "eth_miso") {
        extern int ethMisoPin;
        ethMisoPin = val.toInt();
        imported++;
      } else if (key == "eth_mosi") {
        extern int ethMosiPin;
        ethMosiPin = val.toInt();
        imported++;
      } else if (key == "eth_sclk") {
        extern int ethSclkPin;
        ethSclkPin = val.toInt();
        imported++;
      } else if (key == "eth_cs") {
        extern int ethCsPin;
        ethCsPin = val.toInt();
        imported++;
      } else if (key == "eth_int") {
        extern int ethIntPin;
        ethIntPin = val.toInt();
        imported++;
      } else if (key == "eth_rst") {
        extern int ethRstPin;
        ethRstPin = val.toInt();
        imported++;
      } else if (key == "eth_addr") {
        extern int ethAddr;
        ethAddr = val.toInt();
        imported++;
      } else if (key == "eth_cto") {
        extern int ethConnectTimeout;
        ethConnectTimeout = val.toInt();
        imported++;
      } else if (key == "mmdvm_rx") {
        extern int mmdvmRxPin;
        mmdvmRxPin = val.toInt();
        imported++;
      } else if (key == "mmdvm_tx") {
        extern int mmdvmTxPin;
        mmdvmTxPin = val.toInt();
        imported++;
      } else if (key == "mmdvm_boot") {
        extern int mmdvmBootPin;
        mmdvmBootPin = val.toInt();
        imported++;
      } else if (key == "mmdvm_rst") {
        extern int mmdvmResetPin;
        mmdvmResetPin = val.toInt();
        imported++;
      } else if (key == "mmdvm_wakeup") {
        extern int mmdvmWakeupPin;
        mmdvmWakeupPin = val.toInt();
        imported++;
      } else if (key == "mmdvm_baud") {
        extern int mmdvmBaudrate;
        mmdvmBaudrate = val.toInt();
        imported++;
      } else if (key == "mmdvm_txdly") {
        extern int mmdvmTxDelay;
        mmdvmTxDelay = val.toInt();
        imported++;
      } else if (key == "mmdvm_rxlvl") {
        extern int mmdvmRxLevel;
        mmdvmRxLevel = val.toInt();
        imported++;
      } else if (key == "mmdvm_txlvl") {
        extern int mmdvmTxLevel;
        mmdvmTxLevel = val.toInt();
        imported++;
      } else if (key == "ota_en") {
        extern bool arduinoOtaEnabled;
        arduinoOtaEnabled = (val == "1");
        imported++;
      } else if (key == "ota_pass") {
        extern String arduinoOtaPassword;
        arduinoOtaPassword = val;
        imported++;
      } else if (key == "ota_port") {
        extern int arduinoOtaPort;
        arduinoOtaPort = val.toInt();
        imported++;
      }
    }

    return imported;
}

// ---------------------------------------------------------------------------
// registerConfigRoutes() — thin HTTP wrappers that delegate to the helpers.
// ---------------------------------------------------------------------------
void registerConfigRoutes()
{
  server.on("/api/export-config", HTTP_GET, []() {
    String config = generateConfigString();
    server.sendHeader("Content-Disposition", "attachment; filename=mmdvm-config.txt");
    server.send(200, "text/plain", config);
  });

  server.on("/api/import-config", HTTP_POST, []() {
    String body = server.arg("plain");
    if (body.length() == 0) {
      server.send(400, "text/plain", "ERROR: No configuration data received");
      return;
    }
    int imported = applyConfigString(body);
    saveSettings();
    addLogMessage("[System] Configuration imported: " + String(imported) + " settings applied");
    server.send(200, "text/plain", "Configuration imported: " + String(imported) + " settings applied");
  });
}
