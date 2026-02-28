/*
 * web_handlers_wg_settings.h - WireGuard VPN Settings Routes Registration
 *
 * Declares registerWgSettingsRoutes() which registers HTTP routes for:
  *   /api/save-wg-service + reset
  *   /api/save-wg-config + reset
 */

#ifndef WEB_HANDLERS_WG_SETTINGS
#define WEB_HANDLERS_WG_SETTINGS

void registerWgSettingsRoutes();

#endif // WEB_HANDLERS_WG_SETTINGS
