// ── Digital watchface ─────────────────────────────────────────────────────────
// Full-screen dark LCD layout with segmented main time,
// day/date header, and dual-time lower row.
// Shared helpers (drawTopLeftIcons, drawBatteryInfo, drawClockDots, CLK_BG,
// CLK_HAND) are defined in clock_screen.h which includes this file.
#pragma once

#define DIG_LCD_X       0
#define DIG_LCD_Y       0
#define DIG_LCD_W     240
#define DIG_LCD_H     240

#define DIG_DIV1_Y     52
#define DIG_DIV2_Y    134
#define DIG_DIV3_Y    182

#define DIG_TOP_Y      34
#define DIG_MAIN_Y    108
#define DIG_DUAL_Y    158

#define DIG_LCD_BG     TFT_BLACK
#define DIG_LCD_GRID   0x2945
#define DIG_TEXT_MAIN  TFT_WHITE
#define DIG_TEXT_DIM   0xAD55

static void digDrawSignalGlyph(int x, int y, uint32_t color) {
  ttgo->tft->drawPixel(x, y, color);
  ttgo->tft->drawPixel(x + 1, y, color);
  ttgo->tft->drawCircle(x + 4, y, 2, color);
  ttgo->tft->drawCircle(x + 8, y, 4, color);
}

static void digDrawStaticSkin() {
  ttgo->tft->fillScreen(CLK_BG);

  ttgo->tft->fillRect(DIG_LCD_X, DIG_LCD_Y, DIG_LCD_W, DIG_LCD_H, DIG_LCD_BG);

  ttgo->tft->drawFastHLine(DIG_LCD_X + 6, DIG_DIV1_Y, DIG_LCD_W - 12, DIG_LCD_GRID);
  ttgo->tft->drawFastHLine(DIG_LCD_X + 6, DIG_DIV2_Y, DIG_LCD_W - 12, DIG_LCD_GRID);
  ttgo->tft->drawFastHLine(DIG_LCD_X + 6, DIG_DIV3_Y, DIG_LCD_W - 12, DIG_LCD_GRID);
}

static void digDrawDynamicRows(const struct tm* shown,
                               bool hasTime,
                               int h24,
                               int m,
                               int s,
                               int dualH,
                               int dualM) {
  ttgo->tft->fillRect(DIG_LCD_X + 4, 20, DIG_LCD_W - 8, 26, DIG_LCD_BG);
  ttgo->tft->fillRect(DIG_LCD_X + 4, 82, DIG_LCD_W - 8, 46, DIG_LCD_BG);
  ttgo->tft->fillRect(DIG_LCD_X + 4, 144, DIG_LCD_W - 8, 30, DIG_LCD_BG);

  static const char* const DOW[] = {
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
  };

  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(DIG_TEXT_MAIN, DIG_LCD_BG);
  ttgo->tft->setTextDatum(ML_DATUM);

  if (hasTime) {
    char dateBuf[8];
    snprintf(dateBuf, sizeof(dateBuf), "%02d-%02d", shown->tm_mon + 1, shown->tm_mday);
    ttgo->tft->drawString(DOW[shown->tm_wday], DIG_LCD_X + 10, DIG_TOP_Y);
    ttgo->tft->setTextDatum(MR_DATUM);
    ttgo->tft->drawString(dateBuf, DIG_LCD_X + DIG_LCD_W - 10, DIG_TOP_Y);
  } else {
    ttgo->tft->drawString("SYNC", DIG_LCD_X + 10, DIG_TOP_Y);
    ttgo->tft->setTextDatum(MR_DATUM);
    ttgo->tft->drawString("--:--", DIG_LCD_X + DIG_LCD_W - 10, DIG_TOP_Y);
  }

  bool pm = (h24 >= 12);
  int h12 = h24 % 12;
  if (h12 == 0) h12 = 12;

  char mainBuf[6];
  snprintf(mainBuf, sizeof(mainBuf), "%02d%c%02d", h12, (s & 1) ? ':' : ' ', m);
  char secBuf[4];
  snprintf(secBuf, sizeof(secBuf), "%02d", s);

  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(DIG_TEXT_DIM, DIG_LCD_BG);
  ttgo->tft->setTextDatum(ML_DATUM);
  ttgo->tft->drawString(pm ? "PM" : "AM", DIG_LCD_X + 10, 94);
  ttgo->tft->setTextDatum(MR_DATUM);
  ttgo->tft->drawString("ALM", DIG_LCD_X + DIG_LCD_W - 10, 94);

  ttgo->tft->setTextFont(7);
  ttgo->tft->setTextColor(DIG_TEXT_MAIN, DIG_LCD_BG);
  ttgo->tft->setTextDatum(ML_DATUM);
  ttgo->tft->drawString(mainBuf, DIG_LCD_X + 20, DIG_MAIN_Y);

  ttgo->tft->setTextFont(4);
  ttgo->tft->setTextDatum(MR_DATUM);
  ttgo->tft->drawString(secBuf, DIG_LCD_X + DIG_LCD_W - 10, DIG_MAIN_Y);

  char dualBuf[6];
  snprintf(dualBuf, sizeof(dualBuf), "%02d:%02d", dualH, dualM);

  digDrawSignalGlyph(DIG_LCD_X + 10, DIG_DUAL_Y - 4, DIG_TEXT_DIM);
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(DIG_TEXT_DIM, DIG_LCD_BG);
  ttgo->tft->setTextDatum(ML_DATUM);
  ttgo->tft->drawString("P", DIG_LCD_X + 26, DIG_DUAL_Y - 1);

  ttgo->tft->setTextFont(4);
  ttgo->tft->setTextColor(DIG_TEXT_MAIN, DIG_LCD_BG);
  ttgo->tft->setTextDatum(MC_DATUM);
  ttgo->tft->drawString(dualBuf, 123, DIG_DUAL_Y);

  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(DIG_TEXT_DIM, DIG_LCD_BG);
  ttgo->tft->setTextDatum(MR_DATUM);
  ttgo->tft->drawString("T2", DIG_LCD_X + DIG_LCD_W - 10, DIG_DUAL_Y - 1);
}

static void drawClockScreenDigital() {
  struct tm t;
  bool hasTime = getClockTime(&t);

  struct tm shown;
  if (hasTime) {
    shown = t;
  } else {
    // Pre-sync: fast-running fake time (20x speed) so the face still feels alive.
    time_t fakeTime = (time_t)(millis() / 50UL);
    gmtime_r(&fakeTime, &shown);
  }

  int h = shown.tm_hour;
  int m = shown.tm_min;
  int s = shown.tm_sec;

  int dualH = (h + 1) % 24;
  int dualM = m;
  if (hasTime) {
    time_t now = time(nullptr);
    struct tm utc;
    gmtime_r(&now, &utc);
    dualH = utc.tm_hour;
    dualM = utc.tm_min;
  }

  if (lastDrawnSecond == -1) {
    digDrawStaticSkin();
  }

  digDrawDynamicRows(&shown, hasTime, h, m, s, dualH, dualM);

  // Keep existing status indicators from the rest of the UI.
  drawTopLeftIcons();
  drawBatteryInfo();
  drawClockDots(228);

  lastDrawnSecond = s;
}
