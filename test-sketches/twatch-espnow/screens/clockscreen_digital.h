// ── Digital watchface ─────────────────────────────────────────────────────────
// Full-screen dark LCD layout with segmented main time,
// day/date header, and last-message lower row.
// Shared helpers (drawTopLeftIcons, drawBatteryInfo, drawClockDots, CLK_BG,
// CLK_HAND) are defined in clock_screen.h which includes this file.
#pragma once

#define DIG_LCD_X       0
#define DIG_LCD_Y       0
#define DIG_LCD_W     240
#define DIG_LCD_H     240

#define DIG_DIV1_Y     52
#define DIG_DIV2_Y    144
#define DIG_DIV3_Y    182

#define DIG_TOP_Y      45
#define DIG_MAIN_Y    108
#define DIG_DUAL_Y    158

#define DIG_LCD_BG     TFT_BLACK
#define DIG_LCD_GRID   0x2945
#define DIG_TEXT_MAIN  TFT_WHITE
#define DIG_TEXT_DIM   0xAD55

static void digDrawStaticSkin() {
  ttgo->tft->fillScreen(CLK_BG);

  ttgo->tft->fillRect(DIG_LCD_X, DIG_LCD_Y, DIG_LCD_W, DIG_LCD_H, DIG_LCD_BG);

  //ttgo->tft->drawFastHLine(DIG_LCD_X + 6, DIG_DIV1_Y, DIG_LCD_W - 12, DIG_LCD_GRID);
  ttgo->tft->drawFastHLine(DIG_LCD_X + 10, DIG_DIV2_Y, DIG_LCD_W - 12, DIG_LCD_GRID);
  //ttgo->tft->drawFastHLine(DIG_LCD_X + 6, DIG_DIV3_Y, DIG_LCD_W - 12, DIG_LCD_GRID);
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
    snprintf(dateBuf, sizeof(dateBuf), "%02d-%02d", shown->tm_mday, shown->tm_mon + 1);
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

static void digDrawModeLabel(bool pm, bool use24h) {
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(TFT_RED, DIG_LCD_BG);
  ttgo->tft->setTextDatum(MR_DATUM);
  ttgo->tft->drawString(use24h ? "24H" : (pm ? "PM" : "AM"), DIG_LCD_X + DIG_LCD_W - 10, 94);
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
  ttgo->tft->setTextDatum(MC_DATUM);
  ttgo->tft->drawString(mainBuf, 90, DIG_MAIN_Y);
}

static void digDrawSec(int s) {
  char secBuf[4];
  snprintf(secBuf, sizeof(secBuf), "%02d", s);
  ttgo->tft->setTextFont(4);
  ttgo->tft->setTextColor(CLK_HAND, DIG_LCD_BG);
  ttgo->tft->setTextDatum(MR_DATUM);
  ttgo->tft->drawString(secBuf, DIG_LCD_X + DIG_LCD_W - 10, 117);
}

// Return how many leading chars from src fit the current font and max width.
static size_t digFitCharsToWidth(const char* src, int maxW) {
  char probe[96];
  size_t n = 0;
  probe[0] = '\0';

  while (src[n] != '\0' && n < sizeof(probe) - 1) {
    probe[n] = src[n];
    probe[n + 1] = '\0';
    if (ttgo->tft->textWidth(probe) > maxW) break;
    n++;
  }
  return n;
}

static void digDrawLastMsgRow() {
  char msgBuf[80];

  if (lastMsg[0] != '\0') {
    strncpy(msgBuf, lastMsg, sizeof(msgBuf) - 1);
    msgBuf[sizeof(msgBuf) - 1] = '\0';
  } else if (lastDmrSrc != 0 || lastDmrDst != 0) {
    snprintf(msgBuf, sizeof(msgBuf), "DMR %u>%u", lastDmrSrc, lastDmrDst);
  } else {
    strncpy(msgBuf, "NO MESSAGES", sizeof(msgBuf) - 1);
    msgBuf[sizeof(msgBuf) - 1] = '\0';
  }

  // Normalize newlines/control chars into spaces for a clean two-line row.
  for (size_t i = 0; msgBuf[i] != '\0'; i++) {
    if ((uint8_t)msgBuf[i] < 32) msgBuf[i] = ' ';
  }

  // Clear only the message band to avoid stale trailing glyphs.
  ttgo->tft->fillRect(DIG_LCD_X + 4, DIG_DUAL_Y - 10, DIG_LCD_W - 8, 68, DIG_LCD_BG);

  const int maxW = DIG_LCD_W - 18;
  // Keep message text at font 4 for readability.
  ttgo->tft->setTextFont(4);
  ttgo->tft->setTextColor(DIG_TEXT_MAIN, DIG_LCD_BG);
  ttgo->tft->setTextDatum(MC_DATUM);

  size_t split = digFitCharsToWidth(msgBuf, maxW);
  if (split == 0) split = 1;

  // Prefer splitting line 1 at the last space to keep words intact.
  if (msgBuf[split] != '\0') {
    size_t back = split;
    while (back > 0 && msgBuf[back] != ' ') back--;
    if (back > 3) split = back;
  }

  char line1[80];
  strncpy(line1, msgBuf, split);
  line1[split] = '\0';
  while (strlen(line1) > 0 && line1[strlen(line1) - 1] == ' ') {
    line1[strlen(line1) - 1] = '\0';
  }

  ttgo->tft->drawString(line1, 120, DIG_DUAL_Y + 16);

  const char* rest = msgBuf + split;
  while (*rest == ' ') rest++;
  if (*rest == '\0') return;

  size_t n2 = digFitCharsToWidth(rest, maxW);
  if (n2 == 0) n2 = 1;

  char line2[80];
  strncpy(line2, rest, n2);
  line2[n2] = '\0';

  bool truncated = (rest[n2] != '\0');
  while (strlen(line2) > 0 && line2[strlen(line2) - 1] == ' ') {
    line2[strlen(line2) - 1] = '\0';
  }
  if (truncated && strlen(line2) > 3) {
    size_t n = strlen(line2);
    line2[n - 3] = '.';
    line2[n - 2] = '.';
    line2[n - 1] = '.';
  }

  ttgo->tft->drawString(line2, 120, DIG_DUAL_Y + 48);
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

  // Main time + AM/PM + message row: on first draw or every minute.
  if (firstDraw || m != digLastMinute) {
    digDrawModeLabel(pm, clock24h);
    digDrawMainTime(h, m, s);
    digDrawLastMsgRow();
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
