// ── WiFi settings sub-screen ──────────────────────────────────────────────────
// Toggle online/offline mode; shows full-screen reboot prompt after toggling.
#pragma once

static void drawWifiSettingsScreen() {
  loadOnlineMode(); // always reload from NVS when entering this screen
  ttgo->tft->fillScreen(TFT_BLACK);
  ttgo->tft->setTextDatum(MC_DATUM);

  if (modeRebootPrompt) {
    // ── Full-screen reboot confirmation ──────────────────────────────────────
    ttgo->tft->setTextFont(4);
    ttgo->tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    ttgo->tft->drawString("Reboot now?", 120, 65);

    ttgo->tft->setTextFont(2);
    ttgo->tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    ttgo->tft->drawString("to apply changes", 120, 100);

    // YES button (left half)
    ttgo->tft->fillRoundRect(10, 140, 100, 80, 14, TFT_GREEN);
    ttgo->tft->setTextFont(4);
    ttgo->tft->setTextColor(TFT_BLACK, TFT_GREEN);
    ttgo->tft->drawString("YES", 60, 180);

    // NO button (right half)
    ttgo->tft->fillRoundRect(130, 140, 100, 80, 14, TFT_RED);
    ttgo->tft->setTextColor(TFT_WHITE, TFT_RED);
    ttgo->tft->drawString("NO", 180, 180);

  } else {
    // ── Normal toggle screen ──────────────────────────────────────────────────
    ttgo->tft->setTextFont(4);
    ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
    ttgo->tft->drawString("WiFi", 120, 28);

    // Show IP and mDNS hostname when online and connected
    if (onlineMode && WiFi.status() == WL_CONNECTED) {
      ttgo->tft->setTextFont(4);
      ttgo->tft->setTextColor(TFT_CYAN, TFT_BLACK);
      ttgo->tft->drawString(WiFi.localIP().toString(), 120, 58);
      ttgo->tft->setTextFont(4);
      ttgo->tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
      ttgo->tft->drawString("twatch-espnow", 120, 90);
      ttgo->tft->drawString(".local", 120, 118);
    } else {
      ttgo->tft->setTextFont(2);
      ttgo->tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
      ttgo->tft->drawString(onlineMode ? "connecting..." : "offline mode", 120, 78);
    }

    // Toggle switch
    int x = 120, y = 158, w = 100, h = 40;
    ttgo->tft->fillRoundRect(x-w/2, y-h/2, w, h, 20, TFT_DARKGREY);
    int knobR = 16;
    int knobX = onlineMode ? (x + w/2 - knobR - 6) : (x - w/2 + knobR + 6);
    uint32_t knobColor = onlineMode ? TFT_GREEN : TFT_ORANGE;
    ttgo->tft->fillCircle(knobX, y, knobR, knobColor);
    ttgo->tft->setTextFont(2);
    ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
    ttgo->tft->drawString("ONLINE",  x + w/2 + 30, y);
    ttgo->tft->drawString("OFFLINE", x - w/2 - 30, y);

    // Back button
    ttgo->tft->fillRoundRect(60, 200, 120, 32, 8, TFT_DARKGREY);
    ttgo->tft->setTextColor(TFT_WHITE, TFT_DARKGREY);
    ttgo->tft->drawString("Back", 120, 216);
  }
}
