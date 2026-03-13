// screens/quicksettings_screen.h
// Quick Settings — full 240×240 screen, reached via long-press of crown button.
//
// Controls:
//   Brightness : [-]  filled bar  [+]
//   Volume     : [-]  filled bar  [+]   (scales vibration; ready for audio)
//   Vibration  : ON / OFF toggle
//   Back       : return to previous screen

// Helper: draw a labelled [-] bar [+] row
// ly = top of the control row (label drawn 12px above)
static void qsDraw3Part(const char* label, int value, int maxVal,
                        uint16_t barColor, int ly) {
  TFT_eSPI *tft = ttgo->tft;
  const int BTN_W = 40, BTN_H = 30, BAR_Y = ly, ROW_H = BTN_H;

  // Label
  tft->setTextDatum(TL_DATUM);
  tft->setTextFont(2);
  tft->setTextColor(TFT_YELLOW, TFT_BLACK);
  tft->drawString(label, 12, ly - 16);

  // [-] button  x=10
  tft->fillRoundRect(10, ly, BTN_W, BTN_H, 7, TFT_NAVY);
  tft->drawRoundRect(10, ly, BTN_W, BTN_H, 7, TFT_WHITE);
  tft->setTextDatum(MC_DATUM);
  tft->setTextFont(4);
  tft->setTextColor(TFT_WHITE, TFT_NAVY);
  tft->drawString("-", 30, ly + BTN_H / 2);

  // [+] button  x=190
  tft->fillRoundRect(190, ly, BTN_W, BTN_H, 7, TFT_NAVY);
  tft->drawRoundRect(190, ly, BTN_W, BTN_H, 7, TFT_WHITE);
  tft->setTextColor(TFT_WHITE, TFT_NAVY);
  tft->drawString("+", 210, ly + BTN_H / 2);

  // Bar  x=58..188  (130px wide)
  tft->fillRoundRect(58, ly + 3, 130, BTN_H - 6, 4, 0x2104);
  tft->drawRoundRect(58, ly + 3, 130, BTN_H - 6, 4, 0x8410);
  int filled = (int)((value / (float)maxVal) * 126);
  if (filled > 0)
    tft->fillRoundRect(60, ly + 5, filled, BTN_H - 10, 3, barColor);

  // Percentage label inside bar
  char buf[8];
  snprintf(buf, sizeof(buf), "%d%%", (int)(value * 100.0f / maxVal));
  tft->setTextFont(2);
  tft->setTextDatum(MC_DATUM);
  tft->setTextColor(filled > 63 ? TFT_BLACK : TFT_WHITE,
                    filled > 63 ? barColor   : 0x2104);
  tft->drawString(buf, 123, ly + BTN_H / 2);
}

// ─── draw ─────────────────────────────────────────────────────────────────────
static void drawQuickSettingsScreen() {
  TFT_eSPI *tft = ttgo->tft;
  tft->fillScreen(TFT_BLACK);

  // Title bar
  tft->fillRect(0, 0, 240, 32, 0x1082);
  tft->setTextDatum(MC_DATUM);
  tft->setTextFont(4);
  tft->setTextColor(TFT_WHITE, 0x1082);
  tft->drawString("Quick Settings", 120, 16);

  // ── Brightness  (label top = 41, control y = 55)
  qsDraw3Part("Brightness", currentBrightness, 255, TFT_CYAN, 55);

  // ── Volume      (label top = 103, control y = 117)
  qsDraw3Part("Vibration", currentVolume, 100, 0xFD20, 117);  // orange bar

  // ── Vibration toggle  y=170..200
  tft->setTextDatum(TL_DATUM);
  tft->setTextFont(2);
  tft->setTextColor(TFT_YELLOW, TFT_BLACK);
  tft->drawString("Vibration", 12, 154);

  uint16_t vbg = vibeEnabled ? 0x03C0 : 0x3186;
  tft->fillRoundRect(10, 170, 220, 30, 9, vbg);
  tft->drawRoundRect(10, 170, 220, 30, 9, TFT_WHITE);
  tft->setTextDatum(MC_DATUM);
  tft->setTextFont(4);
  tft->setTextColor(TFT_WHITE, vbg);
  tft->drawString(vibeEnabled ? "ON" : "OFF", 120, 185);

  // ── Back button  y=206..236 (same size as vibration toggle)
  tft->fillRoundRect(10, 206, 220, 30, 9, 0x7800);
  tft->drawRoundRect(10, 206, 220, 30, 9, TFT_WHITE);
  tft->setTextFont(2);
  tft->setTextDatum(MC_DATUM);
  tft->setTextColor(TFT_WHITE, 0x7800);
  tft->drawString("BACK", 120, 221);
}

// ─── touch handler ────────────────────────────────────────────────────────────
static void quickSettingsHandleTouch(int16_t tx, int16_t ty) {
  // ── Brightness row  y=55..85 ────────────────────────────────────────────────
  if (ty >= 55 && ty <= 85) {
    if (tx >= 10 && tx <= 50) {                          // [-]
      if (currentBrightness > 50) currentBrightness -= 25;
      else                         currentBrightness  = 25;
    } else if (tx >= 190 && tx <= 230) {                 // [+]
      if (currentBrightness < 230) currentBrightness += 25;
      else                          currentBrightness  = 255;
    }
    ttgo->bl->adjust(currentBrightness);
    saveBrightnessVibe();
    needsRedraw = true;
    return;
  }

  // ── Volume row  y=117..147 ──────────────────────────────────────────────────
  if (ty >= 117 && ty <= 147) {
    if (tx >= 10 && tx <= 50) {                          // [-]
      if (currentVolume >= 10) currentVolume -= 10;
      else                      currentVolume  = 0;
    } else if (tx >= 190 && tx <= 230) {                 // [+]
      if (currentVolume <= 90) currentVolume += 10;
      else                      currentVolume  = 100;
    }
    saveBrightnessVibe();
    needsRedraw = true;
    return;
  }

  // ── Vibration toggle  y=170..200 ────────────────────────────────────────────
  if (ty >= 170 && ty <= 200) {
    vibeEnabled = !vibeEnabled;
    saveBrightnessVibe();
    needsRedraw = true;
    return;
  }

  // ── Back  y=206..236 ────────────────────────────────────────────────────────
  if (ty >= 206 && ty <= 236) {
    currentScreen   = prevScreen;
    lastDrawnSecond = -1;
    needsRedraw     = true;
  }
}
