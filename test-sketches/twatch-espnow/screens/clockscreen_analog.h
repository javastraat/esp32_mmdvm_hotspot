// ── Analog watchface ──────────────────────────────────────────────────────────
// Dark Style-Seven analog clock. Black face, white ticks + numerals,
// royal-blue hands, red second hand. Incremental erase avoids full-face redraws.
// Shared helpers (drawTopLeftIcons, drawBatteryInfo, drawClockDots, drawDateBoxes,
// CLK_BG, CLK_HAND) are defined in clock_screen.h which includes this file.
#pragma once

#define CLK_TICK   TFT_WHITE   // tick marks and hour numerals
#define TICK_OUTER 113         // outer radius of tick ring (fits 240px screen)
#define NUM_RADIUS  88         // radius to centre of hour numerals

static void fillHand(float cx, float cy, float angleDeg,
                     float front, float back, float width, uint32_t color) {
  float rad  = (angleDeg - 90.0f) * DEG_TO_RAD;
  float cosA = cosf(rad), sinA = sinf(rad);
  float px   = -sinA, py = cosA;
  float hw   = width / 2.0f;

  float x0 = cx - back*cosA + hw*px,  y0 = cy - back*sinA + hw*py;
  float x1 = cx - back*cosA - hw*px,  y1 = cy - back*sinA - hw*py;
  float x2 = cx + front*cosA + hw*px, y2 = cy + front*sinA + hw*py;
  float x3 = cx + front*cosA - hw*px, y3 = cy + front*sinA - hw*py;

  ttgo->tft->fillTriangle(x0, y0, x1, y1, x2, y2, color);
  ttgo->tft->fillTriangle(x1, y1, x2, y2, x3, y3, color);
}

static const char* const HOUR_LABEL[12] = {
  "12","1","2","3","4","5","6","7","8","9","10","11"
};

static void drawHourNum(int idx) {
  float angle = (idx * 30.0f - 90.0f) * DEG_TO_RAD;
  int x = 120 + (int)(NUM_RADIUS * cosf(angle));
  int y = 120 + (int)(NUM_RADIUS * sinf(angle));
  ttgo->tft->setTextDatum(MC_DATUM);
  ttgo->tft->setTextFont(4);
  ttgo->tft->setTextColor(CLK_TICK, CLK_BG);
  ttgo->tft->drawString(HOUR_LABEL[idx], x, y);
}

// 60 tick marks: hour ticks are thicker (3 parallel lines), minute ticks single
static void drawTicks() {
  for (int i = 0; i < 60; i++) {
    bool  isHour = (i % 5 == 0);
    float angle  = (i * 6.0f - 90.0f) * DEG_TO_RAD;
    int   len    = isHour ? 13 : 6;
    float ca = cosf(angle), sa = sinf(angle);
    int x1 = 120 + (int)(TICK_OUTER * ca),        y1 = 120 + (int)(TICK_OUTER * sa);
    int x2 = 120 + (int)((TICK_OUTER-len) * ca),  y2 = 120 + (int)((TICK_OUTER-len) * sa);
    ttgo->tft->drawLine(x1, y1, x2, y2, CLK_TICK);
    if (isHour) {
      int px = -(int)(sa + 0.5f), py = (int)(ca + 0.5f);
      ttgo->tft->drawLine(x1+px, y1+py, x2+px, y2+py, CLK_TICK);
      ttgo->tft->drawLine(x1-px, y1-py, x2-px, y2-py, CLK_TICK);
    }
  }
}

static void drawClockStaticFace() {
  drawTicks();
  for (int i = 0; i < 12; i++) drawHourNum(i);
}

// Erase old hands (draw in black), then restore ticks + numerals + date
static void eraseClockHands(const struct tm* t) {
  if (prevHourAngle < -900) return;
  const int CX = 120, CY = 120;

  fillHand(CX, CY, prevHourAngle,   55, 14, 13, CLK_BG);
  fillHand(CX, CY, prevMinuteAngle, 78, 14, 10, CLK_BG);

  float sRad = (prevSecondAngle - 90.0f) * DEG_TO_RAD;
  int tipX  = CX + (int)(85 * cosf(sRad));
  int tipY  = CY + (int)(85 * sinf(sRad));
  int tailX = CX - (int)(20 * cosf(sRad));
  int tailY = CY - (int)(20 * sinf(sRad));
  for (int d = -2; d <= 2; d++) {
    ttgo->tft->drawLine(tailX+d, tailY, tipX+d, tipY, CLK_BG);
    ttgo->tft->drawLine(tailX, tailY+d, tipX, tipY+d, CLK_BG);
  }
  ttgo->tft->fillCircle(CX, CY, 9, CLK_BG);

  drawTicks();
  for (int i = 0; i < 12; i++) drawHourNum(i);
  if (t) drawDateBoxes(t);   // drawDateBoxes defined in clock_screen.h
}

static void drawClockHands(int h, int m, int s) {
  const int CX = 120, CY = 120;

  prevHourAngle   = (h % 12) * 30.0f + m * 0.5f;
  prevMinuteAngle = m * 6.0f + s * 0.1f;
  prevSecondAngle = s * 6.0f;

  fillHand(CX, CY, prevHourAngle,   55, 14, 10, CLK_HAND);
  fillHand(CX, CY, prevMinuteAngle, 78, 14,  7, CLK_HAND);

  float sRad = (prevSecondAngle - 90.0f) * DEG_TO_RAD;
  int tipX  = CX + (int)(85 * cosf(sRad));
  int tipY  = CY + (int)(85 * sinf(sRad));
  int tailX = CX - (int)(20 * cosf(sRad));
  int tailY = CY - (int)(20 * sinf(sRad));
  ttgo->tft->drawLine(tailX,   tailY,   tipX,   tipY,   TFT_RED);
  ttgo->tft->drawLine(tailX+1, tailY,   tipX+1, tipY,   TFT_RED);
  ttgo->tft->drawLine(tailX,   tailY+1, tipX,   tipY+1, TFT_RED);

  ttgo->tft->fillCircle(CX, CY, 7, CLK_HAND);
  ttgo->tft->fillCircle(CX, CY, 3, TFT_WHITE);
}

static void drawClockScreenAnalog() {
  struct tm t;
  bool hasTime = getClockTime(&t);

  if (lastDrawnSecond == -1) {
    ttgo->tft->fillScreen(CLK_BG);
    drawClockStaticFace();
    if (hasTime) drawDateBoxes(&t);
    drawTopLeftIcons();
    drawClockDots();
    drawBatteryInfo();
    prevHourAngle = prevMinuteAngle = prevSecondAngle = -999.0f;
  } else {
    eraseClockHands(hasTime ? &t : nullptr);
    drawTopLeftIcons();
    drawClockDots();
    drawBatteryInfo();
  }

  if (hasTime) {
    drawClockHands(t.tm_hour, t.tm_min, t.tm_sec);
    lastDrawnSecond = t.tm_sec;
  } else {
    time_t fakeTime = (time_t)(millis() / 50UL);
    struct tm f;
    gmtime_r(&fakeTime, &f);
    drawClockHands(f.tm_hour, f.tm_min, f.tm_sec);
    lastDrawnSecond = f.tm_sec;
  }
}
