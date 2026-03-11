// ── DMR screen ────────────────────────────────────────────────────────────────
#pragma once

static void drawDmrScreen() {
  ttgo->tft->fillScreen(TFT_BLACK);
  ttgo->tft->setTextDatum(TC_DATUM);

  ttgo->tft->setTextColor(TFT_GREEN, TFT_BLACK);
  ttgo->tft->setTextFont(2);
  ttgo->tft->drawString("DMR", 120, 18);
  ttgo->tft->drawFastHLine(50, 38, 140, TFT_DARKGREY);

  if (lastDmrSrc > 0) {
    ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
    ttgo->tft->setTextFont(4);
    ttgo->tft->drawString(String(lastDmrSrc), 120, 52);

    ttgo->tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    ttgo->tft->setTextFont(2);
    ttgo->tft->drawString("to", 120, 94);

    ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
    ttgo->tft->setTextFont(4);
    ttgo->tft->drawString(String(lastDmrDst), 120, 112);

    char info[16];
    snprintf(info, sizeof(info), "TS%u  %s", lastDmrSlot, lastDmrGroup ? "TG" : "PC");
    ttgo->tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    ttgo->tft->setTextFont(2);
    ttgo->tft->drawString(info, 120, 162);
  } else {
    ttgo->tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    ttgo->tft->setTextFont(2);
    ttgo->tft->drawString("no data", 120, 110);
  }

  drawPageDots();
}
