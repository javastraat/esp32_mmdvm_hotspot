// screens/quicksettings_screen.h
// Quick Settings — full 240×240 screen, reached via long-press of crown button.
//
// Controls:
//   Brightness : [-]  filled bar  [+]
//   Vibration  : ON / OFF toggle
//   Back       : return to previous screen

// ─── draw ─────────────────────────────────────────────────────────────────────
static void drawQuickSettingsScreen() {
  TFT_eSPI *tft = ttgo->tft;
  tft->fillScreen(TFT_BLACK);

  // Title bar
  tft->fillRect(0, 0, 240, 36, 0x1082);          // very dark blue-grey
  tft->setTextDatum(MC_DATUM);
  tft->setTextFont(4);
  tft->setTextColor(TFT_WHITE, 0x1082);
  tft->drawString("Quick Settings", 120, 18);

  // ── Brightness ──────────────────────────────────────────────────────────────
  tft->setTextDatum(TL_DATUM);
  tft->setTextFont(2);
  tft->setTextColor(TFT_YELLOW, TFT_BLACK);
  tft->drawString("Brightness", 12, 52);

  // [-] button  x=10, y=74, w=44, h=36
  tft->fillRoundRect(10, 74, 44, 36, 8, TFT_NAVY);
  tft->drawRoundRect(10, 74, 44, 36, 8, TFT_WHITE);
  tft->setTextDatum(MC_DATUM);
  tft->setTextFont(4);
  tft->setTextColor(TFT_WHITE, TFT_NAVY);
  tft->drawString("-", 32, 92);

  // [+] button  x=186, y=74, w=44, h=36
  tft->fillRoundRect(186, 74, 44, 36, 8, TFT_NAVY);
  tft->drawRoundRect(186, 74, 44, 36, 8, TFT_WHITE);
  tft->setTextColor(TFT_WHITE, TFT_NAVY);
  tft->drawString("+", 208, 92);

  // Bar background  x=62, y=80, w=116, h=24
  tft->fillRoundRect(62, 80, 116, 24, 4, 0x2104);
  tft->drawRoundRect(62, 80, 116, 24, 4, 0x8410);
  int filled = (int)((currentBrightness / 255.0f) * 112);
  if (filled > 0)
    tft->fillRoundRect(64, 82, filled, 20, 3, TFT_CYAN);

  // Numeric percentage label
  char pct[8];
  snprintf(pct, sizeof(pct), "%d%%", (int)(currentBrightness / 255.0f * 100));
  tft->setTextDatum(MC_DATUM);
  tft->setTextFont(2);
  tft->setTextColor(filled > 60 ? TFT_BLACK : TFT_WHITE, filled > 60 ? TFT_CYAN : 0x2104);
  tft->drawString(pct, 120, 92);

  // ── Vibration ───────────────────────────────────────────────────────────────
  tft->setTextDatum(TL_DATUM);
  tft->setTextFont(2);
  tft->setTextColor(TFT_YELLOW, TFT_BLACK);
  tft->drawString("Vibration", 12, 132);

  uint16_t vbg = vibeEnabled ? 0x03C0 : 0x3186;   // green or mid-grey
  tft->fillRoundRect(10, 154, 220, 40, 10, vbg);
  tft->drawRoundRect(10, 154, 220, 40, 10, TFT_WHITE);
  tft->setTextDatum(MC_DATUM);
  tft->setTextFont(4);
  tft->setTextColor(TFT_WHITE, vbg);
  tft->drawString(vibeEnabled ? "ON" : "OFF", 120, 174);

  // ── Back button ─────────────────────────────────────────────────────────────
  tft->fillRoundRect(10, 208, 220, 28, 10, 0x7800);   // dark red
  tft->drawRoundRect(10, 208, 220, 28, 10, TFT_WHITE);
  tft->setTextDatum(MC_DATUM);
  tft->setTextFont(2);
  tft->setTextColor(TFT_WHITE, 0x7800);
  tft->drawString("BACK", 120, 222);
}

// ─── touch handler ────────────────────────────────────────────────────────────
static void quickSettingsHandleTouch(int16_t tx, int16_t ty) {
  // [-] brightness  x=10..54, y=74..110
  if (tx >= 10 && tx <= 54 && ty >= 74 && ty <= 110) {
    if (currentBrightness > 50) currentBrightness -= 50;
    else                         currentBrightness  = 25;
    ttgo->bl->adjust(currentBrightness);
    saveBrightnessVibe();
    needsRedraw = true;
    return;
  }

  // [+] brightness  x=186..230, y=74..110
  if (tx >= 186 && tx <= 230 && ty >= 74 && ty <= 110) {
    if (currentBrightness < 205) currentBrightness += 50;
    else                          currentBrightness  = 255;
    ttgo->bl->adjust(currentBrightness);
    saveBrightnessVibe();
    needsRedraw = true;
    return;
  }

  // Vibe toggle  x=10..230, y=154..194
  if (tx >= 10 && tx <= 230 && ty >= 154 && ty <= 194) {
    vibeEnabled = !vibeEnabled;
    saveBrightnessVibe();
    needsRedraw = true;
    return;
  }

  // Back  x=10..230, y=208..236
  if (ty >= 208) {
    currentScreen   = prevScreen;
    lastDrawnSecond = -1;
    needsRedraw     = true;
  }
}
