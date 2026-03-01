/*
 * web_handlers_dmr_settings.h - DMR/Radio/Hotspot Settings Routes Registration
 *
 * Declares registerDmrSettingsRoutes() which registers HTTP routes for:
  *   /api/oled-framebuffer  - OLED framebuffer readout
  *   /api/mode-toggle       - enable/disable radio modes
  *   /api/save-callsign + reset
  *   /api/save-station + reset
  *   /api/save-rf-settings + reset
  *   /api/save-dmr-network + reset
  *   /api/save-hotspot + reset
 */

#ifndef WEB_HANDLERS_DMR_SETTINGS
#define WEB_HANDLERS_DMR_SETTINGS

void registerDmrSettingsRoutes();

#endif // WEB_HANDLERS_DMR_SETTINGS
