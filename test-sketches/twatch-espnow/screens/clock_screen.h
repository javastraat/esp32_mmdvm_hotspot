// ── Clock screen ──────────────────────────────────────────────────────────────
// Dark Style-Seven analog clock. Black face, white ticks + numerals,
// royal-blue hands, red second hand. Incremental erase avoids full-face redraws.
#pragma once

#define CLK_BG    TFT_BLACK   // face background
#define CLK_TICK  TFT_WHITE   // tick marks and hour numerals
#define CLK_HAND  0x435B      // royal blue (#4169E1) — hour + minute hands
#define CLK_DATE  0x435B      // date box border and text
#define TICK_OUTER 113        // outer radius of tick ring (fits 240px screen)
#define NUM_RADIUS  88        // radius to center of hour numerals

// Small WiFi icon for clock status bar (dot + 3 arcs, ~20×14px).
static void drawWifiIconSmall(int cx, int cy, uint32_t color) {
  const int ay = cy + 6;   // arc anchor / dot Y
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

// Small satellite icon — body + two solar wings + antenna with dish dot.
// cx/cy = centre of the body. Fits in ~26×16px.
static void drawSatelliteIcon(int cx, int cy, uint32_t color) {
  // Body (7×7)
  ttgo->tft->fillRect(cx - 3, cy - 3, 7, 7, color);

  // Left solar wing (9×3)
  ttgo->tft->fillRect(cx - 13, cy - 1, 9, 3, color);

  // Right solar wing (9×3)
  ttgo->tft->fillRect(cx + 5,  cy - 1, 9, 3, color);

  // Antenna: diagonal line up-right from body corner
  ttgo->tft->drawLine(cx + 3, cy - 3, cx + 8, cy - 8, color);

  // Dish dot at antenna tip
  ttgo->tft->fillCircle(cx + 8, cy - 8, 2, color);
}

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

// Full static face: ticks + numerals (called once on first show)
static void drawClockStaticFace() {
  drawTicks();
  for (int i = 0; i < 12; i++) drawHourNum(i);
}

// Single date line above clock centre: "TUE 11 MAR" in white
#define DATE_Y  90   // y-centre of date line (above clock centre at y=120)

static void drawDateBoxes(const struct tm* t) {
  static const char* const MON[] = {
    "JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};
  static const char* const DOW[] = {
    "SUN","MON","TUE","WED","THU","FRI","SAT"};

  char buf[14];
  snprintf(buf, sizeof(buf), "%s %d %s", DOW[t->tm_wday], t->tm_mday, MON[t->tm_mon]);

  // Clear previous text area
  ttgo->tft->fillRect(50, DATE_Y - 9, 140, 18, CLK_BG);

  ttgo->tft->setTextDatum(MC_DATUM);
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(TFT_WHITE, CLK_BG);
  ttgo->tft->drawString(buf, 120, DATE_Y);
}

// Erase old hands (draw in black), then restore ticks + numerals + date boxes
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

  // Restore static face elements swept by hands
  drawTicks();
  for (int i = 0; i < 12; i++) drawHourNum(i);
  if (t) drawDateBoxes(t);
}

static void drawClockHands(int h, int m, int s) {
  const int CX = 120, CY = 120;

  prevHourAngle   = (h % 12) * 30.0f + m * 0.5f;
  prevMinuteAngle = m * 6.0f + s * 0.1f;
  prevSecondAngle = s * 6.0f;

  fillHand(CX, CY, prevHourAngle,   55, 14, 10, CLK_HAND);
  fillHand(CX, CY, prevMinuteAngle, 78, 14,  7, CLK_HAND);

  // Second hand — thin red line
  float sRad = (prevSecondAngle - 90.0f) * DEG_TO_RAD;
  int tipX  = CX + (int)(85 * cosf(sRad));
  int tipY  = CY + (int)(85 * sinf(sRad));
  int tailX = CX - (int)(20 * cosf(sRad));
  int tailY = CY - (int)(20 * sinf(sRad));
  ttgo->tft->drawLine(tailX,   tailY,   tipX,   tipY,   TFT_RED);
  ttgo->tft->drawLine(tailX+1, tailY,   tipX+1, tipY,   TFT_RED);
  ttgo->tft->drawLine(tailX,   tailY+1, tipX,   tipY+1, TFT_RED);

  // Center: blue circle with white centre dot
  ttgo->tft->fillCircle(CX, CY, 7, CLK_HAND);
  ttgo->tft->fillCircle(CX, CY, 3, TFT_WHITE);
}

static void drawClockDots() {
  for (int i = 0; i < SCREEN_COUNT; i++) {
    int x = 120 + (int)((i - (SCREEN_COUNT - 1) / 2.0) * 12);
    if (i == currentScreen)
      ttgo->tft->fillCircle(x, 220, 3, CLK_HAND);
    else
      ttgo->tft->drawCircle(x, 220, 3, TFT_DARKGREY);
  }
}

static void drawClockScreen() {
  struct tm t;
  bool hasTime = getClockTime(&t);

  if (lastDrawnSecond == -1) {
    // First show: black fill + static face + date + overlay indicators
    ttgo->tft->fillScreen(CLK_BG);
    drawClockStaticFace();
    if (hasTime) drawDateBoxes(&t);
    if (espnowSynced)                              drawSatelliteIcon(215, 10, TFT_RED);
    if (onlineMode && WiFi.status()==WL_CONNECTED) drawWifiIconSmall(215,  8, TFT_GREEN);
    drawClockDots();
    prevHourAngle = prevMinuteAngle = prevSecondAngle = -999.0f;
  } else {
    // Incremental: erase old hands + restore what they covered
    eraseClockHands(hasTime ? &t : nullptr);
    if (espnowSynced)                              drawSatelliteIcon(215, 10, TFT_RED);
    if (onlineMode && WiFi.status()==WL_CONNECTED) drawWifiIconSmall(215,  8, TFT_GREEN);
    drawClockDots();
  }

  if (hasTime) {
    drawClockHands(t.tm_hour, t.tm_min, t.tm_sec);
    lastDrawnSecond = t.tm_sec;
  } else {
    // Pre-sync: fast-spinning hands (20× speed)
    time_t fakeTime = (time_t)(millis() / 50UL);
    struct tm f;
    gmtime_r(&fakeTime, &f);
    drawClockHands(f.tm_hour, f.tm_min, f.tm_sec);
    lastDrawnSecond = f.tm_sec;
  }
}
