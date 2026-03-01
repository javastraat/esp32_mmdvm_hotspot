/*
 * web_handlers_network_settings.cpp - Network/NTP/WiFi-AP/Ethernet-DNS Settings Routes
 *
 * Extracted from web_handlers_settings.cpp.
 * All routes registered here use the global `server` object (extern WebServer server).
 *
 * Routes:
  *   /api/save-network-settings + reset  - mDNS, web port, DNS fallback
  *   /api/save-time-settings + reset     - NTP config
  *   /api/save-wifi-ap-settings + reset  - WiFi AP mode
  *   /api/save-eth-dns-settings + reset  - Ethernet enable & DNS fallback
 */

#include "system/web_handlers_network_settings.h"
#include "system/system_webserver.h"
#include "system/system_logger.h"
#include "include/config.h"

extern bool mdnsEnabled;
extern String mdnsHostname;
extern bool dnsFallbackEnabled;
extern String dnsFallbackIp;
extern bool ntpEnabled;
extern String ntpServer;
extern int32_t ntpGmtOffsetSec;
extern int32_t ntpDaylightOffsetSec;
extern uint32_t ntpSyncIntervalMs;
extern String wifiApSsid;
extern String wifiApPassword;
extern uint8_t wifiApChannel;
extern uint8_t wifiMaxRetries;
extern bool ethEnabled;
extern bool ethDebug;
extern void saveSettings();

void registerNetworkSettingsRoutes()
{
  // Network Settings (mDNS and Web Server)
  server.on("/api/save-network-settings", HTTP_POST, []()
            {
    if (!server.hasArg("mdns") || !server.hasArg("hostname")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }
    bool mdns = (server.arg("mdns") == "1");
    String hostname = server.arg("hostname");
    bool dnsFb = (server.arg("dnsfallback") == "1");
    String dnsFbIp = server.arg("dnsfallbackip");

    if (hostname.length() < 3) {
      server.send(400, "text/plain", "ERROR: Hostname too short");
      return;
    }

    mdnsEnabled = mdns;
    mdnsHostname = hostname;
    dnsFallbackEnabled = dnsFb;
    dnsFallbackIp = dnsFbIp;
    saveSettings();
    addLogMessage("[Settings] Network settings updated");
    server.send(200, "text/plain", "Network settings saved"); });

  server.on("/api/reset-network-settings", HTTP_POST, []()
            {
    mdnsEnabled = ENABLE_MDNS;
    mdnsHostname = MDNS_HOSTNAME;
    dnsFallbackEnabled = DNS_FALLBACK_ENABLED;
    dnsFallbackIp = DNS_FALLBACK_IP;
    saveSettings();
    addLogMessage("[Settings] Network settings reset to default");
    server.send(200, "text/plain", "Network settings reset to default"); });

  // Time Settings (NTP)
  server.on("/api/save-time-settings", HTTP_POST, []()
            {
    if (!server.hasArg("ntpenabled") || !server.hasArg("server") || !server.hasArg("gmt") || !server.hasArg("dst") || !server.hasArg("sync")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }
    bool ntpEn = (server.arg("ntpenabled") == "1");
    String server_str = server.arg("server");
    int32_t gmt = server.arg("gmt").toInt();
    int32_t dst = server.arg("dst").toInt();
    uint32_t sync = server.arg("sync").toInt();

    if (server_str.length() < 3) {
      server.send(400, "text/plain", "ERROR: NTP server name too short");
      return;
    }
    if (sync < 60000 || sync > 86400000) {
      server.send(400, "text/plain", "ERROR: Sync interval must be 60000-86400000 ms");
      return;
    }

    ntpEnabled = ntpEn;
    ntpServer = server_str;
    ntpGmtOffsetSec = gmt;
    ntpDaylightOffsetSec = dst;
    ntpSyncIntervalMs = sync;
    saveSettings();
    addLogMessage("[Settings] Time settings updated");
    server.send(200, "text/plain", "Time settings saved"); });

  server.on("/api/reset-time-settings", HTTP_POST, []()
            {
    ntpEnabled = NTP_ENABLED;
    ntpServer = NTP_SERVER;
    ntpGmtOffsetSec = NTP_GMT_OFFSET_SEC;
    ntpDaylightOffsetSec = NTP_DAYLIGHT_OFFSET_SEC;
    ntpSyncIntervalMs = NTP_SYNC_INTERVAL_MS;
    saveSettings();
    addLogMessage("[Settings] Time settings reset to default");
    server.send(200, "text/plain", "Time settings reset to default"); });

  // WiFi AP Settings endpoints
  server.on("/api/save-wifi-ap-settings", HTTP_POST, []()
            {
    if (server.hasArg("ssid") && server.hasArg("password") && server.hasArg("channel") && server.hasArg("retries")) {
      String ssid = server.arg("ssid");
      String password = server.arg("password");
      uint8_t channel = server.arg("channel").toInt();
      uint8_t retries = server.arg("retries").toInt();

      // Validate inputs
      if (ssid.length() < 3) {
        server.send(400, "text/plain", "WiFi AP SSID must be at least 3 characters");
        return;
      }
      if (password.length() < 8) {
        server.send(400, "text/plain", "WiFi AP password must be at least 8 characters");
        return;
      }
      if (channel < 1 || channel > 13) {
        server.send(400, "text/plain", "WiFi AP channel must be 1-13");
        return;
      }
      if (retries < 1 || retries > 20) {
        server.send(400, "text/plain", "Max retries must be 1-20");
        return;
      }

      wifiApSsid = ssid;
      wifiApPassword = password;
      wifiApChannel = channel;
      wifiMaxRetries = retries;
      saveSettings();
      addLogMessage("[Settings] WiFi AP settings saved: SSID=" + ssid + ", Channel=" + String(channel) + ", Retries=" + String(retries));
      server.send(200, "text/plain", "WiFi AP settings saved");
    } else {
      server.send(400, "text/plain", "Missing parameters");
    } });

  server.on("/api/reset-wifi-ap-settings", HTTP_POST, []()
            {
    wifiApSsid = WIFI_AP_SSID;
    wifiApPassword = WIFI_AP_PASSWORD;
    wifiApChannel = WIFI_AP_CHANNEL;
    wifiMaxRetries = WIFI_MAX_RETRIES;
    saveSettings();
    addLogMessage("[Settings] WiFi AP settings reset to default");
    server.send(200, "text/plain", "WiFi AP settings reset to default"); });

  // Ethernet & DNS Settings endpoints
  server.on("/api/save-eth-dns-settings", HTTP_POST, []()
            {
    if (server.hasArg("ethenabled") && server.hasArg("ethdebug") && server.hasArg("dnsfallback") && server.hasArg("dnsfallbackip")) {
      bool ethEn = (server.arg("ethenabled") == "1");
      bool ethDb = (server.arg("ethdebug") == "1");
      bool dnsFb = (server.arg("dnsfallback") == "1");
      String dnsFbIp = server.arg("dnsfallbackip");

      // Basic IP validation
      if (dnsFbIp.length() < 7 || dnsFbIp.length() > 15) {
        server.send(400, "text/plain", "Invalid DNS fallback IP address");
        return;
      }

      ethEnabled = ethEn;
      ethDebug = ethDb;
      dnsFallbackEnabled = dnsFb;
      dnsFallbackIp = dnsFbIp;
      saveSettings();
      addLogMessage("[Settings] Ethernet/DNS settings saved: ETH=" + String(ethEn) + ", Debug=" + String(ethDb) + ", DNS Fallback=" + String(dnsFb) + ", IP=" + dnsFbIp);
      server.send(200, "text/plain", "Ethernet/DNS settings saved");
    } else {
      server.send(400, "text/plain", "Missing parameters");
    } });

  server.on("/api/reset-eth-dns-settings", HTTP_POST, []()
            {
    ethEnabled = ETH_ENABLED;
    ethDebug = ETH_DEBUG;
    dnsFallbackEnabled = DNS_FALLBACK_ENABLED;
    dnsFallbackIp = DNS_FALLBACK_IP;
    saveSettings();
    addLogMessage("[Settings] Ethernet/DNS settings reset to default");
    server.send(200, "text/plain", "Ethernet/DNS settings reset to default"); });

}
