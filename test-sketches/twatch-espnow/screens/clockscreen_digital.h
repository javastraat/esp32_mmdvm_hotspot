// ── Digital watchface ─────────────────────────────────────────────────────────
// Full-screen digital clock: large 7-segment HH:MM, seconds below, date above.
// Shared helpers (drawTopLeftIcons, drawBatteryInfo, drawClockDots, drawDateBoxes,
// CLK_BG, CLK_HAND) are defined in clock_screen.h which includes this file.
#pragma once

#define DIG_DATE_Y   42    // y-centre of date line
#define DIG_TIME_Y  115    // y-centre of HH:MM (font 7 = 48px)
#define DIG_SEC_Y   163    // y-centre of seconds

static void drawClockScreenDigital() {
  struct tm t;
  bool hasTime = getClockTime(&t);

  int h = 0, m = 0, s = 0;
  if (hasTime) {
    h = t.tm_hour; m = t.tm_min; s = t.tm_sec;
  } else {
    // Pre-sync: fast-spinning hands (20× speed)
    time_t fakeTime = (time_t)(millis() / 50UL);
    struct tm f;
    gmtime_r(&fakeTime, &f);
    h = f.tm_hour; m = f.tm_min; s = f.tm_sec;
  }

  if (lastDrawnSecond == -1) {
    // Full redraw on first show
    ttgo->tft->fillScreen(CLK_BG);
    if (hasTime) drawDateBoxes(&t, DIG_DATE_Y);
  } else {
    // Incremental: only refresh date area (handles month roll etc.)
    if (hasTime) drawDateBoxes(&t, DIG_DATE_Y);
  }

  // Status bar and dots (self-clearing)
  drawTopLeftIcons();
  drawBatteryInfo();
  drawClockDots();

  // ── HH:MM — font 7 (7-segment 48px), colon blinks on even seconds ──────────
  char timeBuf[6];
  snprintf(timeBuf, sizeof(timeBuf), "%02d%c%02d", h, (s & 1) ? ':' : ' ', m);
  ttgo->tft->setTextDatum(MC_DATUM);
  ttgo->tft->setTextFont(7);
  ttgo->tft->setTextColor(TFT_WHITE, CLK_BG);
  ttgo->tft->drawString(timeBuf, 120, DIG_TIME_Y);

  // ── Seconds — smaller, in accent colour ─────────────────────────────────────
  char secBuf[4];
  snprintf(secBuf, sizeof(secBuf), "%02d", s);
  ttgo->tft->setTextFont(4);
  ttgo->tft->setTextColor(CLK_HAND, CLK_BG);
  ttgo->tft->drawString(secBuf, 120, DIG_SEC_Y);

  lastDrawnSecond = s;
}
