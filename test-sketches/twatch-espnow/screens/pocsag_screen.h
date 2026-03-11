// ── POCSAG screen + shared page-dot indicator ─────────────────────────────────
#pragma once

// Page dots shown on POCSAG and DMR screens (filled = active, outline = inactive)
static void drawPageDots() {
  int cx = 120, y = 220;
  for (int i = 0; i < SCREEN_COUNT; i++) {
    int x = cx + (int)((i - (SCREEN_COUNT - 1) / 2.0) * 12);
    if (i == currentScreen)
      ttgo->tft->fillCircle(x, y, 4, TFT_WHITE);
    else
      ttgo->tft->drawCircle(x, y, 4, TFT_DARKGREY);
  }
}

static void drawPocsagScreen() {
  ttgo->tft->fillScreen(TFT_BLACK);
  ttgo->tft->setTextDatum(TC_DATUM);

  ttgo->tft->setTextColor(TFT_ORANGE, TFT_BLACK);
  ttgo->tft->setTextFont(2);
  ttgo->tft->drawString("POCSAG", 120, 18);
  ttgo->tft->drawFastHLine(50, 38, 140, TFT_DARKGREY);

  if (lastRic > 0) {
    ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
    ttgo->tft->setTextFont(4);
    String msg = String(lastMsg);
    int y = 52;
    for (int offset = 0; offset < (int)msg.length() && offset < 27; offset += 9) {
      ttgo->tft->drawString(msg.substring(offset, offset + 9), 120, y);
      y += 32;
    }
    char ricLine[20];
    snprintf(ricLine, sizeof(ricLine), "RIC %u", lastRic);
    ttgo->tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    ttgo->tft->setTextFont(2);
    ttgo->tft->drawString(ricLine, 120, 168);
  } else {
    ttgo->tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    ttgo->tft->setTextFont(2);
    ttgo->tft->drawString("no data", 120, 110);
  }

  drawPageDots();
}
