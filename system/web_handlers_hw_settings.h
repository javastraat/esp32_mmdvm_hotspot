/*
 * web_handlers_hw_settings.h - Hardware Settings Routes (LED, OLED, SD card, OTA, Security, Ethernet SPI) Registration
 *
 * Declares registerHwSettingsRoutes() which registers HTTP routes for:
  *   /api/oled-power (GET/POST)       - query / toggle OLED display power
  *   /api/save-led-button-settings + reset
  *   /api/save-oled-settings + reset
  *   /reboot
  *   /api/save-eth-settings + reset    - Ethernet SPI pins
  *   /api/save-sdcard-settings + reset
  *   /api/save-ota-settings + reset    - ArduinoOTA config
  *   /api/save-security-settings + reset - web auth credentials
 */

#ifndef WEB_HANDLERS_HW_SETTINGS
#define WEB_HANDLERS_HW_SETTINGS

void registerHwSettingsRoutes();

#endif // WEB_HANDLERS_HW_SETTINGS
