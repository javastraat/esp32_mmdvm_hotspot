// screens/quicksettings_screen.h
// Quick-settings overlay — shown on long press of crown button
//
// Controls:
//   Brightness : [-] filled bar [+]
//   Vibration  : ON / OFF toggle
//   Close      : dismiss overlay

// ─── draw ─────────────────────────────────────────────────────────────────────
static void drawQuickSettingsScreen() {
  TFT_eSPI *tft = ttgo->tft;

  // Overlay panel
  const int OX = 20, OY = 25, OW = 200, OH = 190;
  tft->fillRoundRect(OX, OY, OW, OH, 12, 0x2104);       // very dark grey
  tft->drawRoundRect(OX, OY, OW, OH, 12, TFT_WHITE);

  // Title
  tft->setTextDatum(TC_DATUM);
  tft->setTextFont(2);
  tft->setTextColor(TFT_WHITE, 0x2104);
  tft->drawString("Quick Settings", 120, OY + 7);

  // Divider
  tft->drawFastHLine(OX + 10, OY + 27, OW - 20, 0x8410);

  // ── Brightness ──────────────────────────────────────────────────────────────
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_YELLOW, 0x2104);
  tft->drawString("Brightness", OX + 10, OY + 33);

  // [-] button  (OX+8, OY+53) → 36×28
  tft->fillRoundRect(OX + 8,       OY + 53, 36, 28, 6, TFT_NAVY);
  tft->drawRoundRect(OX + 8,       OY + 53, 36, 28, 6, TFT_WHITE);
  tft->setTextDatum(MC_DATUM);
  tft->setTextColor(TFT_WHITE, TFT_NAVY);
  tft->drawString("-", OX + 8 + 18, OY + 53 + 14);

  // [+] button  (OX+156, OY+53) → 36×28
  tft->fillRoundRect(OX + 156,     OY + 53, 36, 28, 6, TFT_NAVY);
  tft->drawRoundRect(OX + 156,     OY + 53, 36, 28, 6, TFT_WHITE);
  tft->setTextColor(TFT_WHITE, TFT_NAVY);
  tft->drawString("+", OX + 156 + 18, OY + 53 + 14);

  // Brightness bar  (OX+48, OY+57) → 104×20
  const int BX = OX + 48, BY = OY + 57, BW = 104, BH = 20;
  tft->fillRect(BX, BY, BW, BH, TFT_BLACK);
  tft->drawRect(BX, BY, BW, BH, 0x8410);
  int filled = (int)((currentBrightness / 255.0f) * (BW - 2));
  if (filled > 0)
    tft->fillRect(BX + 1, BY + 1, filled, BH - 2, TFT_CYAN);

  // ── Vibration ───────────────────────────────────────────────────────────────
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_YELLOW, 0x2104);
  tft->drawString("Vibration", OX + 10, OY + 95);

  uint16_t vbg = vibeEnabled ? 0x03E0 : 0x4208;   // green or dark grey
  tft->fillRoundRect(OX + 60, OY + 113, 80, 30, 8, vbg);
  tft->drawRoundRect(OX + 60, OY + 113, 80, 30, 8, TFT_WHITE);
  tft->setTextDatum(MC_DATUM);
  tft->setTextColor(TFT_WHITE, vbg);
  tft->drawString(vibeEnabled ? "ON" : "OFF", OX + 60 + 40, OY + 113 + 15);

  // ── Close ────────────────────────────────────────────────────────────────────
  tft->fillRoundRect(OX + 40, OY + 158, 120, 24, 8, 0x7800);   // dark red
  tft->drawRoundRect(OX + 40, OY + 158, 120, 24, 8, TFT_WHITE);
  tft->setTextDatum(MC_DATUM);
  tft->setTextColor(TFT_WHITE, 0x7800);
  tft->drawString("CLOSE", 120, OY + 158 + 12);
}

// ─── touch handler ────────────────────────────────────────────────────────────
static void quickSettingsHandleTouch(int16_t tx, int16_t ty) {
  const int OX = 20, OY = 25, OW = 200, OH = 190;

  // [-] brightness  (OX+8 … OX+44, OY+53 … OY+81)
  if (tx >= OX+8 && tx <= OX+44 && ty >= OY+53 && ty <= OY+81) {
    if (currentBrightness > 50) currentBrightness -= 50;
    else                         currentBrightness  = 25;
    ttgo->bl->adjust(currentBrightness);
    saveBrightnessVibe();
    needsRedraw = true;
    return;
  }

  // [+] brightness  (OX+156 … OX+192, OY+53 … OY+81)
  if (tx >= OX+156 && tx <= OX+192 && ty >= OY+53 && ty <= OY+81) {
    if (currentBrightness < 205) currentBrightness += 50;
    else                          currentBrightness  = 255;
    ttgo->bl->adjust(currentBrightness);
    saveBrightnessVibe();
    needsRedraw = true;
    return;
  }

  // Vibe toggle  (OX+60 … OX+140, OY+113 … OY+143)
  if (tx >= OX+60 && tx <= OX+140 && ty >= OY+113 && ty <= OY+143) {
    vibeEnabled = !vibeEnabled;
    saveBrightnessVibe();
    needsRedraw = true;
    return;
  }

  // Close button  (OX+40 … OX+160, OY+158 … OY+182)
  if (tx >= OX+40 && tx <= OX+160 && ty >= OY+158 && ty <= OY+182) {
    quickSettingsOpen = false;
    lastDrawnSecond   = -1;
    needsRedraw       = true;
    return;
  }

  // Tap outside panel → close
  if (tx < OX || tx > OX+OW || ty < OY || ty > OY+OH) {
    quickSettingsOpen = false;
    lastDrawnSecond   = -1;
    needsRedraw       = true;
  }
}
