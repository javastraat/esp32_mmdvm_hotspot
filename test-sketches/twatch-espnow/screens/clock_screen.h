// ── Clock screen — dispatcher + shared helpers ────────────────────────────────
// Shared constants and drawing helpers used by both watchfaces.
// Include order: shared helpers → clockscreen_analog.h → clockscreen_digital.h
// so that both sub-faces can call drawDateBoxes, drawTopLeftIcons etc.
#pragma once

#define CLK_BG    TFT_BLACK   // face background
#define CLK_HAND  0x435B      // royal blue (#4169E1) — hour + minute hands
#define CLK_DATE  0x435B      // date text colour
#define DATE_Y    90          // default y-centre of date line (used by analog)

// ── Shared icon helpers ───────────────────────────────────────────────────────

// Small WiFi icon for clock status bar (dot + 3 arcs, ~20×14px).
static void drawWifiIconSmall(int cx, int cy, uint32_t color) {
  const int ay = cy + 6;
  ttgo->tft->fillCircle(cx, ay, 2, color);
  const int radii[] = { 5, 9, 13 };
  for (int i = 0; i < 3; i++) {
    int r = radii[i];
    for (int a = -55; a <= 55; a++) {
      float rad = (a - 90.0f) * DEG_TO_RAD;
      int x = cx + (int)(r * cosf(rad));
      int y = ay + (int)(r * sinf(rad));
      ttgo->tft->drawPixel(x, y,   color);
      ttgo->tft->drawPixel(x, y+1, color);
    }
  }
}

// Small pager icon — landscape body, screen outline, two side buttons.
static void drawPagerIcon(int cx, int cy, uint32_t color) {
  ttgo->tft->drawRect(cx - 12, cy - 7, 24, 14, color);
  ttgo->tft->drawRect(cx - 10, cy - 5, 14, 10, color);
  ttgo->tft->fillRect(cx + 6,  cy - 4,  4,  4, color);
  ttgo->tft->fillRect(cx + 6,  cy + 1,  4,  4, color);
}

// Top-left status icons — packed left with no gaps.
static void drawTopLeftIcons() {
  ttgo->tft->fillRect(0, 0, 90, 20, CLK_BG);
  int cx = 20;
  const int step = 32;
  if (onlineMode && WiFi.status() == WL_CONNECTED) {
    drawWifiIconSmall(cx, 8, TFT_GREEN);
    cx += step;
  }
  if (espnowSynced) {
    drawPagerIcon(cx, 10, TFT_RED);
  }
}

// Battery icon + % right-aligned at top-right corner.
static void drawBatteryInfo() {
  ttgo->tft->fillRect(155, 1, 84, 18, CLK_BG);
  if (!ttgo->power->isBatteryConnect()) return;

  int  pct      = (int)ttgo->power->getBattPercentage();
  bool charging = ttgo->power->isChargeing();
  uint32_t col  = pct > 50 ? TFT_GREEN : (pct > 20 ? TFT_YELLOW : TFT_RED);

  char buf[8];
  snprintf(buf, sizeof(buf), "%d%%", pct);
  ttgo->tft->setTextFont(1);
  int textW = ttgo->tft->textWidth(buf);

  const int by = 6;
  int bx = 237 - 20 - 2 - textW;

  ttgo->tft->drawRect(bx, by, 18, 9, TFT_WHITE);
  ttgo->tft->fillRect(bx + 18, by + 2, 2, 5, TFT_WHITE);
  int fw = (15 * pct) / 100;
  if (fw > 0) ttgo->tft->fillRect(bx + 1, by + 1, fw, 7, col);

  if (charging) {
    ttgo->tft->drawFastHLine(bx + 6, by + 4, 6, TFT_WHITE);
    ttgo->tft->drawFastVLine(bx + 9, by + 2, 5, TFT_WHITE);
  }

  ttgo->tft->setTextColor(col, CLK_BG);
  ttgo->tft->setTextDatum(ML_DATUM);
  ttgo->tft->drawString(buf, bx + 22, by + 4);
}

// Navigation dots — centred, one per tap-cycle screen.
// y defaults to 180 (analog); digital face passes its own y.
static void drawClockDots(int y = 180) {
  for (int i = 0; i < SCREEN_COUNT; i++) {
    int x = 120 + (int)((i - (SCREEN_COUNT - 1) / 2.0) * 12);
    if (i == currentScreen)
      ttgo->tft->fillCircle(x, y, 3, CLK_HAND);
    else
      ttgo->tft->drawCircle(x, y, 3, TFT_DARKGREY);
  }
}

// Date line — "TUE 11 MAR". y defaults to DATE_Y (analog); digital passes its own y.
static void drawDateBoxes(const struct tm* t, int y = DATE_Y) {
  static const char* const MON[] = {
    "JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};
  static const char* const DOW[] = {
    "SUN","MON","TUE","WED","THU","FRI","SAT"};

  char buf[14];
  snprintf(buf, sizeof(buf), "%s %d %s", DOW[t->tm_wday], t->tm_mday, MON[t->tm_mon]);

  ttgo->tft->fillRect(50, y - 9, 140, 18, CLK_BG);
  ttgo->tft->setTextDatum(MC_DATUM);
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(TFT_WHITE, CLK_BG);
  ttgo->tft->drawString(buf, 120, y);
}

// ── Watchface sub-files (included after shared helpers) ───────────────────────
#include "clockscreen_analog.h"
#include "clockscreen_digital.h"

// ── Dispatcher ────────────────────────────────────────────────────────────────
static void drawClockScreen() {
  if (watchfaceId == 1)
    drawClockScreenDigital();
  else
    drawClockScreenAnalog();
}
