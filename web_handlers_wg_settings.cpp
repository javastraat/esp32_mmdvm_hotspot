/*
 * web_handlers_wg_settings.cpp - WireGuard VPN Settings Routes
 *
 * Extracted from web_handlers_settings.cpp.
 * All routes registered here use the global `server` object (extern WebServer server).
 *
 * Routes:
  *   /api/save-wg-service + reset
  *   /api/save-wg-config + reset
 */

#include "system/web_handlers_wg_settings.h"
#include "system/system_webserver.h"
#include "system/system_logger.h"
#include "include/config.h"

extern bool wgEnabled;
extern String wgLocalIp;
extern String wgPrivateKey;
extern String wgPublicKey;
extern String wgEndpoint;
extern uint16_t wgEndpointPort;
extern String wgDns;
extern String wgAllowedIps;
extern void saveSettings();

void registerWgSettingsRoutes()
{
  // WireGuard VPN Settings
  server.on("/api/save-wg-service", HTTP_POST, []()
            {
    if (!server.hasArg("enabled")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }
    wgEnabled = (server.arg("enabled") == "1");
    saveSettings();
    addLogMessage("[Settings] WireGuard " + String(wgEnabled ? "enabled" : "disabled"));
    server.send(200, "text/plain", "WireGuard " + String(wgEnabled ? "enabled" : "disabled")); });

  server.on("/api/reset-wg-service", HTTP_POST, []()
            {
    wgEnabled = WG_ENABLED;
    saveSettings();
    addLogMessage("[Settings] WireGuard service reset to default");
    server.send(200, "text/plain", "WireGuard service reset to default"); });

  server.on("/api/save-wg-config", HTTP_POST, []()
            {
    if (!server.hasArg("local_ip") || !server.hasArg("private_key") ||
        !server.hasArg("public_key") || !server.hasArg("endpoint") || !server.hasArg("port") ||
        !server.hasArg("dns") || !server.hasArg("allowed_ips")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }
    String localIp = server.arg("local_ip");
    String privateKey = server.arg("private_key");
    String publicKey = server.arg("public_key");
    String endpoint = server.arg("endpoint");
    uint16_t port = server.arg("port").toInt();
    String dns = server.arg("dns");
    String allowedIps = server.arg("allowed_ips");

    if (localIp.isEmpty()) {
      server.send(400, "text/plain", "ERROR: Local IP is required");
      return;
    }
    if (privateKey.isEmpty()) {
      server.send(400, "text/plain", "ERROR: Private key is required");
      return;
    }
    if (publicKey.isEmpty()) {
      server.send(400, "text/plain", "ERROR: Public key is required");
      return;
    }
    if (endpoint.isEmpty()) {
      server.send(400, "text/plain", "ERROR: Endpoint is required");
      return;
    }
    if (port < 1 || port > 65535) {
      server.send(400, "text/plain", "ERROR: Port must be 1-65535");
      return;
    }

    wgLocalIp = localIp;
    wgPrivateKey = privateKey;
    wgPublicKey = publicKey;
    wgEndpoint = endpoint;
    wgEndpointPort = port;
    wgDns = dns;
    wgAllowedIps = allowedIps;
    saveSettings();

    addLogMessage("[Settings] WireGuard config saved: " + wgEndpoint + ":" + String(wgEndpointPort) + ", DNS: " + wgDns + ", Allowed IPs: " + wgAllowedIps);
    server.send(200, "text/plain", "WireGuard configuration saved"); });

  server.on("/api/reset-wg-config", HTTP_POST, []()
            {
    wgLocalIp = WG_LOCAL_IP;
    wgPrivateKey = WG_PRIVATE_KEY;
    wgPublicKey = WG_PUBLIC_KEY;
    wgEndpoint = WG_ENDPOINT;
    wgEndpointPort = WG_ENDPOINT_PORT;
    wgDns = WG_DNS;
    wgAllowedIps = WG_ALLOWED_IPS;
    saveSettings();
    addLogMessage("[Settings] WireGuard configuration reset to default");
    server.send(200, "text/plain", "WireGuard configuration reset to default"); });

}
