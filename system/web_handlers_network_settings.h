/*
 * web_handlers_network_settings.h - Network/NTP/WiFi-AP/Ethernet-DNS Settings Routes Registration
 *
 * Declares registerNetworkSettingsRoutes() which registers HTTP routes for:
  *   /api/save-network-settings + reset  - mDNS, web port, DNS fallback
  *   /api/save-time-settings + reset     - NTP config
  *   /api/save-wifi-ap-settings + reset  - WiFi AP mode
  *   /api/save-eth-dns-settings + reset  - Ethernet enable & DNS fallback
 */

#ifndef WEB_HANDLERS_NETWORK_SETTINGS
#define WEB_HANDLERS_NETWORK_SETTINGS

void registerNetworkSettingsRoutes();

#endif // WEB_HANDLERS_NETWORK_SETTINGS
