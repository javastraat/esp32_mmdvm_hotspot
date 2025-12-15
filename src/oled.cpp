/*
 * oled.cpp - OLED Display Control
 *
 * Implementation of OLED display functions for status indication
 */

#include "../include/oled.h"

// ===== OLED Display Functions =====
#if ENABLE_OLED
void setupOLED() {
  if (!enable_oled) return;

  // Create mutex for display access protection
  displayMutex = xSemaphoreCreateMutex();
  if (displayMutex == NULL) {
    logSerial("[OLED] Failed to create display mutex!");
  } else {
    logSerial("[OLED] Display mutex created");
  }

  // Initialize I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    logSerial("[OLED] SSD1306 allocation failed!");
    return;
  }
  logSerial("[OLED] Display initialized successfully");

  // Display boot logos
  //
  //Bitmap logo first
  displayBitmap();
  logSerial("[OLED] Display logo bitmap");
  delay(5000);  // Show logo for 2 seconds
  //
  // Then ESP32 logo
  //displayESP32Logo();
  //logSerial("[OLED] Showing ESP32 logo");
  //delay(2000);  // Show logo for 2 seconds
  //
  // boot logo last
  displayBootLogo();
  logSerial("[OLED] Showing boot screen");
}


void displayBitmap(void) {
  display.clearDisplay();

  display.drawBitmap(
    (display.width()  - LOGO_WIDTH ) / 2,
    (display.height() - LOGO_HEIGHT) / 2,
    logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(1000);
}

void displayESP32Logo() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Draw decorative top border
  for (int i = 0; i < OLED_WIDTH; i += 4) {
    display.drawPixel(i, 0, SSD1306_WHITE);
    display.drawPixel(i + 1, 1, SSD1306_WHITE);
  }

  // ESP32 text - large and centered
  display.setTextSize(3);
  String esp32Text = "ESP32";
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(esp32Text, 0, 0, &x1, &y1, &w, &h);
  int16_t x = (OLED_WIDTH - w) / 2;
  display.setCursor(x, 15);
  display.println(esp32Text);

  // MMDVM Hotspot text - smaller, centered
  display.setTextSize(1);
  String mmdvmText = "MMDVM Hotspot";
  display.getTextBounds(mmdvmText, 0, 0, &x1, &y1, &w, &h);
  x = (OLED_WIDTH - w) / 2;
  display.setCursor(x, 45);
  display.println(mmdvmText);

  // Draw decorative bottom border
  for (int i = 0; i < OLED_WIDTH; i += 4) {
    display.drawPixel(i, OLED_HEIGHT - 2, SSD1306_WHITE);
    display.drawPixel(i + 1, OLED_HEIGHT - 1, SSD1306_WHITE);
  }

  display.display();
}

void displayBootLogo() {
  display.clearDisplay();

  // Set text properties
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Title with callsign - centered
  String title = "ESP32 Hotspot";
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  int16_t x = (OLED_WIDTH - w) / 2;
  display.setCursor(x, 0);
  display.println(title);

  // Draw a line
  display.drawLine(0, 10, OLED_WIDTH, 10, SSD1306_WHITE);

  // Firmware version
  display.setCursor(0, 14);
  display.println("Version:");
  display.setCursor(0, 24);
  display.println(FIRMWARE_VERSION);

  // Authors
  display.setCursor(0, 34);
  display.println("Made by:");
  display.setCursor(0, 44);
  display.println("PD2EMC & PD8JO");

  // Status
  display.setCursor(0, 55);
  display.println("Booting...");

  display.display();

  logSerial("[OLED] Boot logo displayed");
}

void updateBootStatus(String status) {
  if (!enable_oled) return;

  // Update only the status line at the bottom of boot screen
  // Clear the status area (bottom line)
  display.fillRect(0, 55, OLED_WIDTH, 9, SSD1306_BLACK);

  // Display new status
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 55);
  display.println(status);

  display.display();

  logSerial("[OLED] " + status);
}

void updateOLEDStatus() {
  if (!enable_oled) return;

  // Try to acquire mutex to prevent SPI conflicts with Ethernet
  if (displayMutex != NULL) {
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
      // Could not acquire mutex in 50ms, skip this update to avoid blocking
      logSerial("[OLED] Skipped update - mutex busy");
      return;
    }
  }

  // Clear entire display for clean look
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // ===== TOP LINE: Date on left, Time centered, Icons on right =====
  // Get current time
  struct tm timeinfo;
  String dateStr = "";
  String timeStr = "";
  if (getLocalTime(&timeinfo)) {
    char dateBuf[8];
    char timeBuf[8];
    strftime(dateBuf, sizeof(dateBuf), "%d/%m", &timeinfo);
    strftime(timeBuf, sizeof(timeBuf), "%H:%M", &timeinfo);
    dateStr = String(dateBuf);
    timeStr = String(timeBuf);
  } else {
    dateStr = "--/--";
    timeStr = "--:--";
  }

  // Display date on the left
  display.setCursor(0, 0);
  display.print(dateStr);

  // Display time centered
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  int16_t timeX = (OLED_WIDTH - w) / 2;
  display.setCursor(timeX, 0);
  display.print(timeStr);

  // Draw icons on the right side of top line (right to left)
  int iconX = OLED_WIDTH - ICON_WIDTH; // Start from right edge

  // Draw network icons - show both if both connected
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
  if (eth_connected && wifiConnected) {
    // Both connected - show both icons
    display.drawBitmap(iconX, 0, icon_ethernet, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
    iconX -= (ICON_WIDTH + 2);
    display.drawBitmap(iconX, 0, icon_wifi, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
    iconX -= (ICON_WIDTH + 2);
  } else if (eth_connected) {
    // Ethernet only
    display.drawBitmap(iconX, 0, icon_ethernet, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
    iconX -= (ICON_WIDTH + 2);
  } else if (wifiConnected) {
    // WiFi only
    display.drawBitmap(iconX, 0, icon_wifi, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
    iconX -= (ICON_WIDTH + 2);
  }
#else
  if (wifiConnected) {
    display.drawBitmap(iconX, 0, icon_wifi, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
    iconX -= (ICON_WIDTH + 2);
  }
#endif

  // Draw antenna/DMR icon (left of network icons)
  if (mode_dmr_enabled && dmrLoggedIn) {
    display.drawBitmap(iconX, 0, icon_antenna, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
  }

  // Draw a line below top row
  display.drawLine(0, 10, OLED_WIDTH, 10, SSD1306_WHITE);

  // DMR Activity Section (between the two lines)
  // Check for active DMR transmission
  bool activityDisplayed = false;

  // Check if both slots are active
  bool bothSlotsActive = dmrActivity[0].active && dmrActivity[1].active;

  // Determine which slot to display
  int slotToDisplay = -1;
  if (bothSlotsActive) {
    // Both active - alternate between them
    slotToDisplay = oledActiveSlot;
    oledActiveSlot = (oledActiveSlot == 0) ? 1 : 0; // Toggle for next update
  } else if (dmrActivity[1].active) {
    // Only Slot 2 active (prioritize Slot 2)
    slotToDisplay = 1;
  } else if (dmrActivity[0].active) {
    // Only Slot 1 active
    slotToDisplay = 0;
  }

  // Display the selected slot
  if (slotToDisplay >= 0) {
    int i = slotToDisplay;

    // Callsign - LARGE and prominent (2x size, centered)
    display.setTextSize(2);
    String callsign = (dmrActivity[i].srcCallsign.length() > 0) ? dmrActivity[i].srcCallsign : "Unknown";
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(callsign, 0, 0, &x1, &y1, &w, &h);
    int16_t x = (OLED_WIDTH - w) / 2;
    display.setCursor(x, 14);
    display.println(callsign);

    // Duration (small text, below callsign)
    display.setTextSize(1);
    display.setCursor(0, 32);
    unsigned long duration = (millis() - dmrActivity[i].startTime) / 1000;
    display.print("Duration: ");
    display.print(duration);
    display.println("s");

    // DMR ID -> Talkgroup with slot indicator at the end (small text)
    display.setCursor(0, 42);
    display.print(dmrActivity[i].srcId);
    display.print(" -> TG ");
    display.print(dmrActivity[i].dstId);
    display.print(" [S");
    display.print(dmrActivity[i].slotNo);
    display.print("]");

    activityDisplayed = true;
  }

  // If no active transmission, show idle state
  if (!activityDisplayed) {
    display.setCursor(0, 20);

    // Check if any mode is enabled
    bool anyModeEnabled = mode_dmr_enabled || mode_dstar_enabled || mode_ysf_enabled ||
                          mode_p25_enabled || mode_nxdn_enabled || mode_pocsag_enabled;

    if (!anyModeEnabled) {
      // No modes activated - center the text
      String line1 = "No mode activated";
      String line2 = "Enable mode in web";

      int16_t x1, y1;
      uint16_t w, h;

      // Center first line
      display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
      int16_t x = (OLED_WIDTH - w) / 2;
      display.setCursor(x, 20);
      display.println(line1);

      // Center second line
      display.getTextBounds(line2, 0, 0, &x1, &y1, &w, &h);
      x = (OLED_WIDTH - w) / 2;
      display.setCursor(x, 30);
      display.println(line2);
    } else if (mode_dmr_enabled) {
      // DMR mode is enabled - show status
      if (dmrLoggedIn) {
        display.println("DMR: Listening");

        // Show current talkgroup if available
        if (currentTalkgroup > 0) {
          display.setCursor(0, 30);
          display.print("TG: ");
          display.println(currentTalkgroup);
        }

        // Show last caller callsign (check both slots for most recent)
        String lastCaller = "";
        unsigned long mostRecentTime = 0;

        for (int i = 0; i < 2; i++) {
          if (dmrActivity[i].srcCallsign.length() > 0 && dmrActivity[i].lastUpdate > mostRecentTime) {
            lastCaller = dmrActivity[i].srcCallsign;
            mostRecentTime = dmrActivity[i].lastUpdate;
          }
        }

        if (lastCaller.length() > 0) {
          display.setCursor(0, 40);
          display.print("Last: ");
          display.println(lastCaller);
        }
      } else {
        // DMR enabled but not connected
        display.println("DMR Mode Active");
        display.setCursor(0, 30);
        display.print("Status: ");
        display.println(dmrLoginStatus);
      }
    } else {
      // Other mode is enabled (D-Star, YSF, P25, NXDN, POCSAG)
      // Show which mode(s) are enabled but not yet implemented
      display.println("Mode enabled:");
      display.setCursor(0, 30);
      if (mode_dstar_enabled) display.println("D-Star (N/A)");
      else if (mode_ysf_enabled) display.println("YSF (N/A)");
      else if (mode_p25_enabled) display.println("P25 (N/A)");
      else if (mode_nxdn_enabled) display.println("NXDN (N/A)");
      else if (mode_pocsag_enabled) display.println("POCSAG (N/A)");
    }
  }

  // ===== BOTTOM ROW: Cycling Network/Callsign info =====
  // Draw a line above bottom status
  display.drawLine(0, 50, OLED_WIDTH, 50, SSD1306_WHITE);

  display.setTextSize(1);

  // Determine what to show based on connections and cycle state
  bool hasWifi = wifiConnected;
  bool hasEth = false;
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
  hasEth = eth_connected;
#endif

  String bottomLine = "";
  bool centerText = false; // Flag to center callsign text

  if (hasWifi && hasEth) {
    // Both connected - cycle through 3 screens
    if (oledHeaderCycle == 0) {
      bottomLine = "WiFi: " + WiFi.localIP().toString();
    } else if (oledHeaderCycle == 1) {
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
      bottomLine = "ETH: " + ETH.localIP().toString();
#endif
    } else {
      bottomLine = dmr_callsign + " - ESP32 HS";
      centerText = true;
    }
  } else if (hasWifi) {
    // WiFi only - alternate between 2 screens
    if (oledHeaderCycle % 2 == 0) {
      bottomLine = "WiFi: " + WiFi.localIP().toString();
    } else {
      bottomLine = dmr_callsign + " - ESP32 HS";
      centerText = true;
    }
  } else if (hasEth) {
    // ETH only - alternate between 2 screens
    if (oledHeaderCycle % 2 == 0) {
#ifdef LILYGO_T_ETH_ELITE_ESP32S3_MMDVM
      bottomLine = "ETH: " + ETH.localIP().toString();
#endif
    } else {
      bottomLine = dmr_callsign + " - ESP32 HS";
      centerText = true;
    }
  } else if (apMode) {
    // AP mode
    if (oledHeaderCycle % 2 == 0) {
      bottomLine = "AP: " + WiFi.softAPIP().toString();
    } else {
      bottomLine = dmr_callsign + " - ESP32 HS";
      centerText = true;
    }
  } else {
    // No network - alternate between warning and callsign
    if (oledHeaderCycle % 2 == 0) {
      bottomLine = "No Network";
      centerText = true;
    } else {
      bottomLine = dmr_callsign + " - ESP32 HS";
      centerText = true;
    }
  }

  // Center the text if it's the callsign, otherwise left-align
  if (centerText) {
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(bottomLine, 0, 0, &x1, &y1, &w, &h);
    int16_t x = (OLED_WIDTH - w) / 2;
    display.setCursor(x, 54);
  } else {
    display.setCursor(0, 54);
  }

  display.print(bottomLine);

  display.display();

  // Release mutex after display update is complete
  if (displayMutex != NULL) {
    xSemaphoreGive(displayMutex);
  }
}

// Control OLED display power (software on/off)
void setOLEDPower(bool on) {
  if (!enable_oled) return;

  if (on) {
    display.ssd1306_command(SSD1306_DISPLAYON);
    oledDisplayOn = true;
    logSerial("[OLED] Display turned ON");
  } else {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    oledDisplayOn = false;
    logSerial("[OLED] Display turned OFF");
  }
}

// Toggle OLED display power
void toggleOLEDPower() {
  setOLEDPower(!oledDisplayOn);
}

#else
// Stub functions when OLED is disabled at compile time
void setupOLED() {
  // No-op when OLED is disabled
}

void updateBootStatus(String status) {
  // No-op when OLED is disabled
}

void updateOLEDStatus() {
  // No-op when OLED is disabled
}

void setOLEDPower(bool on) {
  // No-op when OLED is disabled
}

void toggleOLEDPower() {
  // No-op when OLED is disabled
}
#endif
