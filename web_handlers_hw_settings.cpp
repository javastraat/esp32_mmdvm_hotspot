/*
 * web_handlers_hw_settings.cpp - Hardware Settings Routes (LED, OLED, SD card, OTA, Security, Ethernet SPI)
 *
 * Extracted from web_handlers_settings.cpp.
 * All routes registered here use the global `server` object (extern WebServer server).
 *
 * Routes:
  *   /api/oled-framebuffer         - raw OLED framebuffer (128x64 monochrome)
  *   /api/save-led-button-settings + reset
  *   /api/save-oled-settings + reset
  *   /api/save-eth-settings + reset    - Ethernet SPI pins
  *   /api/save-sdcard-settings + reset
  *   /api/save-ota-settings + reset    - ArduinoOTA config
  *   /api/save-security-settings + reset - web auth credentials
 */

#include "system/web_handlers_hw_settings.h"
#include "system/system_webserver.h"
#include "system/system_logger.h"
#include "system/system_oled.h"
#include "include/config.h"
#include "include/sdcard_handlers.h"

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
extern bool ethEnabled;
extern bool ethDebug;
extern bool webEnabled;
extern uint16_t webServerPort;
extern String webUsername;
extern String webPassword;
extern void saveSettings();

void registerHwSettingsRoutes()
{
  // OLED framebuffer API - returns raw 1024 bytes (128x64 monochrome)
  server.on("/api/oled-framebuffer", HTTP_GET, []()
            {
    uint8_t *buffer = display.getBuffer();
    if (buffer) {
      server.send_P(200, "application/octet-stream", (const char *)buffer, OLED_WIDTH * OLED_HEIGHT / 8);
    } else {
      server.send(503, "text/plain", "OLED not initialized");
    } });

  // OLED power toggle — mirrors button-press behaviour
  server.on("/api/oled-power", HTTP_POST, []()
  {
    extern volatile bool oledDisplayOn;
    oledDisplayOn = !oledDisplayOn;
    if (oledDisplayOn) {
      display.ssd1306_command(SSD1306_DISPLAYON);
      addLogMessage("[OLED] Display turned ON via web");
    } else {
      display.ssd1306_command(SSD1306_DISPLAYOFF);
      addLogMessage("[OLED] Display turned OFF via web");
    }
    server.send(200, "application/json",
                String("{\"on\":") + (oledDisplayOn ? "true" : "false") + "}");
  });

  // OLED power state query
  server.on("/api/oled-power", HTTP_GET, []()
  {
    extern volatile bool oledDisplayOn;
    server.send(200, "application/json",
                String("{\"on\":") + (oledDisplayOn ? "true" : "false") + "}");
  });

  // LED/Button Settings endpoints
  server.on("/api/save-led-button-settings", HTTP_POST, []()
            {
    if (server.hasArg("ledpin") && server.hasArg("buttonpin")) {
      int led = server.arg("ledpin").toInt();
      int btn = server.arg("buttonpin").toInt();
      if (led < 0 || led > 48) {
        server.send(400, "text/plain", "LED pin must be 0-48");
        return;
      }
      if (btn < 0 || btn > 48) {
        server.send(400, "text/plain", "Button pin must be 0-48");
        return;
      }
      ledPin = led;
      buttonPin = btn;
      saveSettings();
      addLogMessage("[Settings] LED/Button settings saved: LED=" + String(led) + ", Button=" + String(btn));
      server.send(200, "text/plain", "LED/Button settings saved");
    } else {
      server.send(400, "text/plain", "Missing parameters");
    } });
  server.on("/api/reset-led-button-settings", HTTP_POST, []()
            {
    ledPin = LED_PIN;
    buttonPin = BUTTON_PIN;
    saveSettings();
    addLogMessage("[Settings] LED/Button settings reset to default");
    server.send(200, "text/plain", "LED/Button settings reset to default"); });

  // OLED Settings endpoints
  server.on("/api/save-oled-settings", HTTP_POST, []()
            {
    if (server.hasArg("oledenabled") && server.hasArg("i2csda") && server.hasArg("i2cscl") && server.hasArg("oledaddr") && server.hasArg("oledwidth") && server.hasArg("oledheight")) {
      bool oledEn = (server.arg("oledenabled") == "1");
      int sda = server.arg("i2csda").toInt();
      int scl = server.arg("i2cscl").toInt();
      // Parse hex address (e.g., "0x3C" or "60")
      String addrStr = server.arg("oledaddr");
      int addr;
      if (addrStr.startsWith("0x") || addrStr.startsWith("0X")) {
        addr = (int)strtol(addrStr.c_str(), NULL, 16);
      } else {
        addr = addrStr.toInt();
      }
      int width = server.arg("oledwidth").toInt();
      int height = server.arg("oledheight").toInt();
      if (sda < 0 || sda > 48) {
        server.send(400, "text/plain", "I2C SDA pin must be 0-48");
        return;
      }
      if (scl < 0 || scl > 48) {
        server.send(400, "text/plain", "I2C SCL pin must be 0-48");
        return;
      }
      if (addr != 0x3C && addr != 0x3D) {
        server.send(400, "text/plain", "OLED I2C address must be 0x3C or 0x3D");
        return;
      }
      if (width < 64 || width > 256) {
        server.send(400, "text/plain", "OLED width must be 64-256");
        return;
      }
      if (height < 32 || height > 128) {
        server.send(400, "text/plain", "OLED height must be 32-128");
        return;
      }
      oledEnabled = oledEn;
      i2cSdaPin = sda;
      i2cSclPin = scl;
      oledI2cAddress = addr;
      oledWidth = width;
      oledHeight = height;
      saveSettings();
      addLogMessage("[Settings] OLED settings saved: Enabled=" + String(oledEn) + ", SDA=" + String(sda) + ", SCL=" + String(scl) + ", Addr=0x" + String(addr, HEX) + ", WxH=" + String(width) + "x" + String(height));
      server.send(200, "text/plain", "OLED settings saved");
    } else {
      server.send(400, "text/plain", "Missing parameters");
    } });
  server.on("/api/reset-oled-settings", HTTP_POST, []()
            {
    oledEnabled = OLED_ENABLED;
    i2cSdaPin = I2C_SDA_PIN;
    i2cSclPin = I2C_SCL_PIN;
    oledI2cAddress = OLED_I2C_ADDRESS;
    oledWidth = OLED_WIDTH;
    oledHeight = OLED_HEIGHT;
    saveSettings();
    addLogMessage("[Settings] OLED settings reset to default");
    server.send(200, "text/plain", "OLED settings reset to default"); });

  // SD Card API handlers
  setupSDCardHandlers(server);

  // Ethernet Settings (simple version for system settings page)
  server.on("/api/save-eth-settings", HTTP_POST, []()
            {
    extern int ethMisoPin;
    extern int ethMosiPin;
    extern int ethSclkPin;
    extern int ethCsPin;
    extern int ethIntPin;
    extern int ethRstPin;
    extern int ethAddr;
    extern int ethConnectTimeout;
    ethEnabled = (server.arg("ethenabled") == "1");
    ethDebug = (server.arg("ethdebug") == "1");
    ethMisoPin = server.arg("ethmiso").toInt();
    ethMosiPin = server.arg("ethmosi").toInt();
    ethSclkPin = server.arg("ethsclk").toInt();
    ethCsPin = server.arg("ethcs").toInt();
    ethIntPin = server.arg("ethint").toInt();
    ethRstPin = server.arg("ethrst").toInt();
    ethAddr = server.arg("ethaddr").toInt();
    ethConnectTimeout = server.arg("ethconntimeout").toInt();
    saveSettings();
    addLogMessage("[Settings] Ethernet settings saved: Enabled=" + String(ethEnabled));
    server.send(200, "text/plain", "Ethernet settings saved"); });
  server.on("/api/reset-eth-settings", HTTP_POST, []()
            {
    extern int ethMisoPin;
    extern int ethMosiPin;
    extern int ethSclkPin;
    extern int ethCsPin;
    extern int ethIntPin;
    extern int ethRstPin;
    extern int ethAddr;
    extern int ethConnectTimeout;
    ethEnabled = ETH_ENABLED;
    ethDebug = ETH_DEBUG;
    ethMisoPin = ETH_MISO_PIN;
    ethMosiPin = ETH_MOSI_PIN;
    ethSclkPin = ETH_SCLK_PIN;
    ethCsPin = ETH_CS_PIN;
    ethIntPin = ETH_INT_PIN;
    ethRstPin = ETH_RST_PIN;
    ethAddr = ETH_ADDR;
    ethConnectTimeout = ETH_CONNECT_TIMEOUT;
    saveSettings();
    addLogMessage("[Settings] Ethernet settings reset to default");
    server.send(200, "text/plain", "Ethernet settings reset to default"); });

  // SD Card Settings
  server.on("/api/save-sdcard-settings", HTTP_POST, []()
            {
    extern bool sdcardEnabled;
    extern int spiMisoPin;
    extern int spiMosiPin;
    extern int spiSclkPin;
    extern int sdCsPin;
    sdcardEnabled = (server.arg("sdcard_en") == "1");
    spiMisoPin = server.arg("spi_miso").toInt();
    spiMosiPin = server.arg("spi_mosi").toInt();
    spiSclkPin = server.arg("spi_sclk").toInt();
    sdCsPin = server.arg("sd_cs").toInt();
    saveSettings();
    addLogMessage("[Settings] SD Card settings saved: Enabled=" + String(sdcardEnabled));
    server.send(200, "text/plain", "SD Card settings saved"); });
  // Arduino OTA Settings endpoints
  server.on("/api/save-ota-settings", HTTP_POST, []()
            {
    if (server.hasArg("enabled") && server.hasArg("password") && server.hasArg("port")) {
      bool otaEnabled = (server.arg("enabled") == "1");
      String otaPassword = server.arg("password");
      int otaPort = server.arg("port").toInt();
      if (otaPort < 1 || otaPort > 65535) {
        server.send(400, "text/plain", "Port must be 1-65535");
        return;
      }
      extern bool arduinoOtaEnabled;
      extern String arduinoOtaPassword;
      extern int arduinoOtaPort;
      arduinoOtaEnabled = otaEnabled;
      arduinoOtaPassword = otaPassword;
      arduinoOtaPort = otaPort;
      saveSettings();
      addLogMessage("[Settings] ArduinoOTA settings saved: Enabled=" + String(otaEnabled) + ", Port=" + String(otaPort));
      server.send(200, "text/plain", "ArduinoOTA settings saved");
    } else {
      server.send(400, "text/plain", "Missing parameters");
    } });
  server.on("/api/reset-ota-settings", HTTP_POST, []()
            {
    extern bool arduinoOtaEnabled;
    extern String arduinoOtaPassword;
    extern int arduinoOtaPort;
    arduinoOtaEnabled = ARDUINO_OTA_ENABLED;
    arduinoOtaPassword = ARDUINO_OTA_PASSWORD;
    arduinoOtaPort = ARDUINO_OTA_PORT;
    saveSettings();
    addLogMessage("[Settings] ArduinoOTA settings reset to default");
    server.send(200, "text/plain", "ArduinoOTA settings reset to default"); });
  server.on("/api/reset-sdcard-settings", HTTP_POST, []()
            {
    extern bool sdcardEnabled;
    extern int spiMisoPin;
    extern int spiMosiPin;
    extern int spiSclkPin;
    extern int sdCsPin;
    sdcardEnabled = SDCARD_ENABLED;
    spiMisoPin = SPI_MISO_PIN;
    spiMosiPin = SPI_MOSI_PIN;
    spiSclkPin = SPI_SCLK_PIN;
    sdCsPin = SD_CS_PIN;
    saveSettings();
    addLogMessage("[Settings] SD Card settings reset to default");
    server.send(200, "text/plain", "SD Card settings reset to default"); });

  // Security Settings endpoints
  server.on("/api/save-security-settings", HTTP_POST, []()
            {
    if (!server.hasArg("webenabled") || !server.hasArg("port") || !server.hasArg("username") || !server.hasArg("password")) {
      server.send(400, "text/plain", "ERROR: Missing parameters");
      return;
    }
    bool authEn = (server.arg("webenabled") == "1");
    int port = server.arg("port").toInt();
    String user = server.arg("username");
    String pass = server.arg("password");
    if (port < 1 || port > 65535) {
      server.send(400, "text/plain", "Web port must be 1-65535");
      return;
    }
    if (user.length() < 1) {
      server.send(400, "text/plain", "Username cannot be empty");
      return;
    }
    if (pass.length() < 1) {
      server.send(400, "text/plain", "Password cannot be empty");
      return;
    }
    extern bool webEnabled;
    extern uint16_t webServerPort;
    extern String webUsername;
    extern String webPassword;
    webEnabled = authEn;
    webServerPort = (uint16_t)port;
    webUsername = user;
    webPassword = pass;
    saveSettings();
    addLogMessage("[Settings] Security settings saved: auth=" + String(authEn ? "on" : "off") + ", port=" + String(port) + ", user=" + user);
    server.send(200, "text/plain", "Security settings saved"); });

  server.on("/api/reset-security-settings", HTTP_POST, []()
            {
    extern bool webEnabled;
    extern uint16_t webServerPort;
    extern String webUsername;
    extern String webPassword;
    webEnabled = WEB_ENABLED;
    webServerPort = WEB_SERVER_PORT;
    webUsername = WEB_USERNAME;
    webPassword = WEB_PASSWORD;
    saveSettings();
    addLogMessage("[Settings] Security settings reset to default");
    server.send(200, "text/plain", "Security settings reset to default"); });

}
