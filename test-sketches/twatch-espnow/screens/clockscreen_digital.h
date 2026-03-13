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

#define DIG_TOP_Y      45
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

// Per-area draw helpers — all use setTextColor(fg, bg) so TFT_eSPI
// overwrites old pixels inline; no fillRect/flash needed.

static int digLastMinute = -1;
static int digLastDay    = -1;

static const char* const DIG_DOW[] = {
  "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};

static void digDrawDateRow(const struct tm* shown, bool hasTime) {
  ttgo->tft->setTextFont(4);
  ttgo->tft->setTextColor(DIG_TEXT_MAIN, DIG_LCD_BG);
  if (hasTime) {
    char dateBuf[8];
    snprintf(dateBuf, sizeof(dateBuf), "%02d-%02d", shown->tm_mon + 1, shown->tm_mday);
    ttgo->tft->setTextDatum(ML_DATUM);
    ttgo->tft->drawString(DIG_DOW[shown->tm_wday], DIG_LCD_X + 10, DIG_TOP_Y);
    ttgo->tft->setTextDatum(MR_DATUM);
    ttgo->tft->drawString(dateBuf, DIG_LCD_X + DIG_LCD_W - 10, DIG_TOP_Y);
  } else {
    ttgo->tft->setTextDatum(ML_DATUM);
    ttgo->tft->drawString("SYNC", DIG_LCD_X + 10, DIG_TOP_Y);
    ttgo->tft->setTextDatum(MR_DATUM);
    ttgo->tft->drawString("--:--", DIG_LCD_X + DIG_LCD_W - 10, DIG_TOP_Y);
  }
}

static void digDrawAmPmAlm(bool pm, bool use24h) {
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(DIG_TEXT_DIM, DIG_LCD_BG);
  ttgo->tft->setTextDatum(ML_DATUM);
  ttgo->tft->drawString(use24h ? "24H" : (pm ? "PM" : "AM"), DIG_LCD_X + 10, 94);
  ttgo->tft->setTextDatum(MR_DATUM);
  ttgo->tft->drawString("ALM", DIG_LCD_X + DIG_LCD_W - 10, 94);
}

static void digDrawMainTime(int h24, int m, int s) {
  int shownHour = h24;
  if (!clock24h) {
    shownHour = h24 % 12;
    if (shownHour == 0) shownHour = 12;
  }

  char mainBuf[6];
  snprintf(mainBuf, sizeof(mainBuf), "%02d%c%02d", shownHour, (s & 1) ? ':' : ' ', m);
  ttgo->tft->setTextFont(7);
  ttgo->tft->setTextColor(DIG_TEXT_MAIN, DIG_LCD_BG);
  ttgo->tft->setTextDatum(MR_DATUM);
  ttgo->tft->drawString(mainBuf, 232, DIG_MAIN_Y);
}

static void digDrawSec(int s) {
  char secBuf[4];
  snprintf(secBuf, sizeof(secBuf), "%02d", s);
  ttgo->tft->setTextFont(4);
  ttgo->tft->setTextColor(CLK_HAND, DIG_LCD_BG);
  ttgo->tft->setTextDatum(ML_DATUM);
  ttgo->tft->drawString(secBuf, DIG_LCD_X + 10, 117);
}

static void digDrawDualRow(int dualH, int dualM) {
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

  bool pm = (h >= 12);

  bool firstDraw = (lastDrawnSecond == -1);

  if (firstDraw) {
    digDrawStaticSkin();
    digLastMinute = -1;
    digLastDay    = -1;
  }

  // Date row: only on first draw or when the calendar day rolls over.
  if (firstDraw || shown.tm_mday != digLastDay) {
    digDrawDateRow(&shown, hasTime);
    digLastDay = shown.tm_mday;
  }

  // Main time + AM/PM + dual time: on first draw or every minute.
  if (firstDraw || m != digLastMinute) {
    digDrawAmPmAlm(pm, clock24h);
    digDrawMainTime(h, m, s);
    digDrawDualRow(dualH, dualM);
    digLastMinute = m;
  } else {
    // Every other second: just blink the colon in-place (tiny overprint).
    digDrawMainTime(h, m, s);
  }

  // Seconds: tiny overprint every tick — no fillRect, no flash.
  digDrawSec(s);

  drawTopLeftIcons();
  drawBatteryInfo();
  drawClockDots(228);

  lastDrawnSecond = s;
}
