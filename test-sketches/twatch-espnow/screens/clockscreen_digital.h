// ── Digital watchface ─────────────────────────────────────────────────────────
// SKMEI-inspired LCD layout with dark layered bezel, segmented main time,
// day/date header, and dual-time lower row.
// Shared helpers (drawTopLeftIcons, drawBatteryInfo, drawClockDots, CLK_BG,
// CLK_HAND) are defined in clock_screen.h which includes this file.
#pragma once

#define DIG_CASE_X     12
#define DIG_CASE_Y     20
#define DIG_CASE_W    216
#define DIG_CASE_H    202

#define DIG_BEZEL_X    20
#define DIG_BEZEL_Y    28
#define DIG_BEZEL_W   200
#define DIG_BEZEL_H   186

#define DIG_LCD_X      28
#define DIG_LCD_Y      38
#define DIG_LCD_W     184
#define DIG_LCD_H     168

#define DIG_DIV1_Y     76
#define DIG_DIV2_Y    134
#define DIG_DIV3_Y    178

#define DIG_TOP_Y      58
#define DIG_MAIN_Y    110
#define DIG_DUAL_Y    156
#define DIG_FOOT_Y    191

#define DIG_CASE_BG    0x1082
#define DIG_CASE_EDGE  0x39E7
#define DIG_BEZEL_BG   0x2104
#define DIG_BEZEL_EDGE 0x6B4D
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

  ttgo->tft->fillRoundRect(DIG_CASE_X, DIG_CASE_Y, DIG_CASE_W, DIG_CASE_H, 20, DIG_CASE_BG);
  ttgo->tft->drawRoundRect(DIG_CASE_X, DIG_CASE_Y, DIG_CASE_W, DIG_CASE_H, 20, DIG_CASE_EDGE);
  ttgo->tft->drawRoundRect(DIG_CASE_X + 1, DIG_CASE_Y + 1, DIG_CASE_W - 2, DIG_CASE_H - 2, 20, 0x18E3);

  ttgo->tft->fillRoundRect(DIG_BEZEL_X, DIG_BEZEL_Y, DIG_BEZEL_W, DIG_BEZEL_H, 14, DIG_BEZEL_BG);
  ttgo->tft->drawRoundRect(DIG_BEZEL_X, DIG_BEZEL_Y, DIG_BEZEL_W, DIG_BEZEL_H, 14, DIG_BEZEL_EDGE);

  ttgo->tft->fillRoundRect(DIG_LCD_X, DIG_LCD_Y, DIG_LCD_W, DIG_LCD_H, 9, DIG_LCD_BG);
  ttgo->tft->drawRoundRect(DIG_LCD_X, DIG_LCD_Y, DIG_LCD_W, DIG_LCD_H, 9, DIG_LCD_GRID);

  ttgo->tft->drawFastHLine(DIG_LCD_X + 6, DIG_DIV1_Y, DIG_LCD_W - 12, DIG_LCD_GRID);
  ttgo->tft->drawFastHLine(DIG_LCD_X + 6, DIG_DIV2_Y, DIG_LCD_W - 12, DIG_LCD_GRID);
  ttgo->tft->drawFastHLine(DIG_LCD_X + 6, DIG_DIV3_Y, DIG_LCD_W - 12, DIG_LCD_GRID);

  ttgo->tft->setTextDatum(MC_DATUM);
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(DIG_TEXT_DIM, DIG_BEZEL_BG);
  ttgo->tft->drawString("SKMEI", 120, 35);

  ttgo->tft->setTextColor(0x630C, DIG_LCD_BG);
  ttgo->tft->drawString("COUNTDOWN", 86, DIG_FOOT_Y);
  ttgo->tft->drawString("DUAL TIME", 160, DIG_FOOT_Y);
}

static void digDrawDynamicRows(const struct tm* shown,
                               bool hasTime,
                               int h24,
                               int m,
                               int s,
                               int dualH,
                               int dualM) {
  ttgo->tft->fillRect(DIG_LCD_X + 4, 46, DIG_LCD_W - 8, 24, DIG_LCD_BG);
  ttgo->tft->fillRect(DIG_LCD_X + 4, 84, DIG_LCD_W - 8, 44, DIG_LCD_BG);
  ttgo->tft->fillRect(DIG_LCD_X + 4, 142, DIG_LCD_W - 8, 30, DIG_LCD_BG);

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
  ttgo->tft->drawString(pm ? "PM" : "AM", DIG_LCD_X + 10, 92);
  ttgo->tft->setTextDatum(MR_DATUM);
  ttgo->tft->drawString("ALM", DIG_LCD_X + DIG_LCD_W - 10, 92);

  ttgo->tft->setTextFont(7);
  ttgo->tft->setTextColor(DIG_TEXT_MAIN, DIG_LCD_BG);
  ttgo->tft->setTextDatum(ML_DATUM);
  ttgo->tft->drawString(mainBuf, DIG_LCD_X + 18, DIG_MAIN_Y);

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
