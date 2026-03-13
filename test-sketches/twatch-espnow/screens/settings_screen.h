// ── Settings screen (3×3 grid) ────────────────────────────────────────────────
#pragma once

#define ICON_BG  0x1082   // dark blue-grey fill for icon bodies

// ── Helper: 3px-thick arc, angles in standard math degrees (0=right, CW) ─────
static void settingsArc(int cx, int cy, int r, int a0, int a1, uint16_t col) {
  for (int dr = -1; dr <= 1; dr++) {
    for (int a = a0; a <= a1; a++) {
      float rad = a * DEG_TO_RAD;
      ttgo->tft->drawPixel(cx + (int)((r + dr) * cosf(rad)),
                           cy + (int)((r + dr) * sinf(rad)), col);
    }
  }
}

// ── WiFi: large dot + 3 thick arcs opening upward ────────────────────────────
static void drawWifiIcon(int cx, int cy, uint32_t color) {
  uint16_t c = (uint16_t)color;
  int ay = cy + 14;
  ttgo->tft->fillCircle(cx, ay, 5, c);
  const int radii[] = { 13, 22, 31 };
  for (int i = 0; i < 3; i++)
    settingsArc(cx, ay, radii[i], -155, -25, c);  // 130° arc centred upward
}

// ── Messages: speech bubble + 3 dots ─────────────────────────────────────────
static void drawMessagesIcon(int cx, int cy, uint32_t color) {
  uint16_t c = (uint16_t)color;
  // Bubble body (filled dark, 2px white border)
  ttgo->tft->fillRoundRect(cx - 22, cy - 16, 44, 28, 8, ICON_BG);
  ttgo->tft->drawRoundRect(cx - 22, cy - 16, 44, 28, 8, c);
  ttgo->tft->drawRoundRect(cx - 21, cy - 15, 42, 26, 8, c);
  // Three dots (typing indicator)
  ttgo->tft->fillCircle(cx - 10, cy - 3, 4, c);
  ttgo->tft->fillCircle(cx,      cy - 3, 4, c);
  ttgo->tft->fillCircle(cx + 10, cy - 3, 4, c);
  // Tail bottom-right
  ttgo->tft->fillTriangle(cx + 6, cy + 12, cx + 20, cy + 12, cx + 20, cy + 24, c);
  ttgo->tft->fillRect(cx + 6, cy + 10, 15, 4, c);  // smooth join to bubble
}

// ── Battery: body + nub + 3 charge segments ──────────────────────────────────
static void drawBatteryIcon(int cx, int cy, uint32_t color) {
  uint16_t c = (uint16_t)color;
  // Body (40×22, rounded, 2px border)
  ttgo->tft->fillRoundRect(cx - 22, cy - 11, 40, 22, 4, ICON_BG);
  ttgo->tft->drawRoundRect(cx - 22, cy - 11, 40, 22, 4, c);
  ttgo->tft->drawRoundRect(cx - 21, cy - 10, 38, 20, 4, c);
  // Nub (right terminal)
  ttgo->tft->fillRect(cx + 18, cy - 5, 6, 10, c);
  // 3 charge segments (≈75% full)
  ttgo->tft->fillRoundRect(cx - 19, cy - 7, 9, 14, 2, c);
  ttgo->tft->fillRoundRect(cx -  8, cy - 7, 9, 14, 2, c);
  ttgo->tft->fillRoundRect(cx +  3, cy - 7, 9, 14, 2, c);
}

// ── Watchface: round clock face with tick marks + hands at 10:10 ─────────────
static void drawWatchfaceLayoutIcon(int cx, int cy, uint32_t color) {
  uint16_t c = (uint16_t)color;
  int R = 25;
  // Filled face + 2px border
  ttgo->tft->fillCircle(cx, cy, R, ICON_BG);
  ttgo->tft->drawCircle(cx, cy, R,     c);
  ttgo->tft->drawCircle(cx, cy, R - 1, c);
  // Tick marks at 12 / 3 / 6 / 9 (2px thick each)
  const int ta[] = { -90, 0, 90, 180 };
  for (int i = 0; i < 4; i++) {
    float r = ta[i] * DEG_TO_RAD;
    int x1 = cx + (int)((R - 2) * cosf(r)), y1 = cy + (int)((R - 2) * sinf(r));
    int x2 = cx + (int)((R - 8) * cosf(r)), y2 = cy + (int)((R - 8) * sinf(r));
    ttgo->tft->drawLine(x1,     y1, x2,     y2, c);
    ttgo->tft->drawLine(x1 + 1, y1, x2 + 1, y2, c);
  }
  // Hour hand (10h10 → 305° from 12 = 215° math), 3px wide
  float hr  = (305.0f - 90.0f) * DEG_TO_RAD;
  int   hLen = (int)(R * 0.55f);
  for (int d = -1; d <= 1; d++)
    ttgo->tft->drawLine(cx + d, cy, cx + d + (int)(hLen * cosf(hr)), cy + (int)(hLen * sinf(hr)), c);
  // Minute hand (60° from 12 = -30° math), 2px wide
  float mr  = (60.0f - 90.0f) * DEG_TO_RAD;
  int   mLen = (int)(R * 0.75f);
  for (int d = 0; d <= 1; d++)
    ttgo->tft->drawLine(cx + d, cy, cx + d + (int)(mLen * cosf(mr)), cy + (int)(mLen * sinf(mr)), c);
  // Centre pip
  ttgo->tft->fillCircle(cx, cy, 3, c);
}

// ── Steps: filled staircase + walking figure ──────────────────────────────────
static void drawStepsIcon(int cx, int cy, uint16_t color) {
  // Staircase: 3 filled columns ascending left → right
  // Column heights: 8, 16, 24 px → step surfaces at cy+10, cy+2, cy-6
  int bx = cx - 18, by = cy + 18;
  ttgo->tft->fillRect(bx,      by - 8,  12, 8,  color);
  ttgo->tft->fillRect(bx + 12, by - 16, 12, 16, color);
  ttgo->tft->fillRect(bx + 24, by - 24, 12, 24, color);

  // Walking figure standing on top step (right side)
  // Feet land at cy-6 (top step surface)
  int fx = cx + 12;
  ttgo->tft->fillCircle(fx, cy - 25, 5, color);                           // head
  ttgo->tft->fillRect(fx - 1, cy - 20, 3, 9, color);                      // torso
  ttgo->tft->drawLine(fx, cy - 17, fx - 6, cy - 11, color);               // left arm fwd
  ttgo->tft->drawLine(fx, cy - 17, fx + 5, cy - 12, color);               // right arm back
  ttgo->tft->drawLine(fx - 1, cy - 11, fx - 5, cy - 6,  color);           // left leg fwd
  ttgo->tft->drawLine(fx + 1, cy - 11, fx + 4, cy - 6,  color);           // right leg back
}

// ── Stopwatch: filled face + crown + tick marks + 2 hands ────────────────────
static void drawStopwatchIcon(int cx, int cy, uint16_t color) {
  int R = 20, oy = cy + 5;
  // Filled face + 2px border
  ttgo->tft->fillCircle(cx, oy, R, ICON_BG);
  ttgo->tft->drawCircle(cx, oy, R,     color);
  ttgo->tft->drawCircle(cx, oy, R - 1, color);
  // Crown stem + button
  ttgo->tft->fillRect(cx - 2, oy - R - 7, 5, 8, color);
  ttgo->tft->fillRoundRect(cx - 8, oy - R - 10, 17, 5, 2, color);
  // Tick marks at 12 / 3 / 6 / 9 (2px)
  const int ta[] = { -90, 0, 90, 180 };
  for (int i = 0; i < 4; i++) {
    float r = ta[i] * DEG_TO_RAD;
    int x1 = cx + (int)((R - 1) * cosf(r)), y1 = oy + (int)((R - 1) * sinf(r));
    int x2 = cx + (int)((R - 6) * cosf(r)), y2 = oy + (int)((R - 6) * sinf(r));
    ttgo->tft->drawLine(x1,     y1, x2,     y2, color);
    ttgo->tft->drawLine(x1 + 1, y1, x2 + 1, y2, color);
  }
  // Minute hand (~12 min, 72° from 12 = -18° math), 2px wide
  float mr  = (72.0f - 90.0f) * DEG_TO_RAD;
  int   mLen = R - 5;
  ttgo->tft->drawLine(cx,     oy, cx     + (int)(mLen * cosf(mr)), oy + (int)(mLen * sinf(mr)), color);
  ttgo->tft->drawLine(cx + 1, oy, cx + 1 + (int)(mLen * cosf(mr)), oy + (int)(mLen * sinf(mr)), color);
  // Second hand (~20 sec, 120° from 12 = 30° math)
  float sr = (120.0f - 90.0f) * DEG_TO_RAD;
  ttgo->tft->drawLine(cx, oy, cx + (int)((R - 3) * cosf(sr)), oy + (int)((R - 3) * sinf(sr)), color);
  // Centre pip
  ttgo->tft->fillCircle(cx, oy, 3, color);
}

// ── Power: bold circle with gap at top + thick stem ──────────────────────────
static void drawPowerIcon(int cx, int cy, uint32_t color) {
  uint16_t c = (uint16_t)color;
  int R = 18, oy = cy + 5;
  // Arc: draw from -60° to 240° (leaves 60° gap at top for the stem)
  settingsArc(cx, oy, R - 1, -60, 240, c);
  settingsArc(cx, oy, R,     -60, 240, c);
  // Stem (5px wide, tall through the gap)
  ttgo->tft->fillRect(cx - 2, oy - R - 6, 5, R + 4, c);
}

// ── Next arrow: bold right-pointing arrow → back to clock ────────────────────
static void drawNextArrowIcon(int cx, int cy, uint32_t color) {
  uint16_t c = (uint16_t)color;
  // Shaft
  ttgo->tft->fillRect(cx - 20, cy - 5, 28, 11, c);
  // Arrowhead: tip fixed at cx+18, widens left
  for (int i = 0; i <= 18; i++) {
    ttgo->tft->drawFastHLine(cx + 18 - i, cy - i, i + 2, c);
    if (i > 0) ttgo->tft->drawFastHLine(cx + 18 - i, cy + i, i + 2, c);
  }
}

// ── Settings screen ────────────────────────────────────────────────────────────
static void drawSettingsScreen() {
  ttgo->tft->fillScreen(TFT_BLACK);

  if (settingsRebootPrompt) {
    ttgo->tft->setTextDatum(MC_DATUM);
    ttgo->tft->setTextFont(4);
    ttgo->tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    ttgo->tft->drawString("Reboot now?", 120, 65);

    ttgo->tft->setTextFont(2);
    ttgo->tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    ttgo->tft->drawString("System restart required", 120, 98);

    ttgo->tft->fillRoundRect(10, 140, 100, 80, 14, TFT_GREEN);
    ttgo->tft->setTextFont(4);
    ttgo->tft->setTextColor(TFT_BLACK, TFT_GREEN);
    ttgo->tft->drawString("YES", 60, 180);

    ttgo->tft->fillRoundRect(130, 140, 100, 80, 14, TFT_RED);
    ttgo->tft->setTextColor(TFT_WHITE, TFT_RED);
    ttgo->tft->drawString("NO", 180, 180);
    return;
  }

  const int cellW = 80, cellH = 80;

  // Short label shown under each icon (empty string = no label)
  const char* iconLabels[3][3] = {
    { "WiFi",      "Messages", "Battery"  },
    { "Watchface", "Steps",    "Timer"    },
    { "",          "Reboot",   "Clock"    }
  };

  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col) {
      int cx = col * cellW;
      int cy = row * cellH;

      // Cell tile: dark background + subtle border
      ttgo->tft->fillRoundRect(cx + 4, cy + 4, cellW - 8, cellH - 8, 12, 0x0841);
      ttgo->tft->drawRoundRect(cx + 4, cy + 4, cellW - 8, cellH - 8, 12, 0x2945);

      // Icon centre — shift up slightly to leave room for the label
      int icx = cx + cellW / 2;
      int icy = cy + cellH / 2 - 6;

      if      (row == 0 && col == 0) drawWifiIcon(icx, icy, TFT_WHITE);
      else if (row == 0 && col == 1) drawMessagesIcon(icx, icy, TFT_WHITE);
      else if (row == 0 && col == 2) drawBatteryIcon(icx, icy, TFT_WHITE);
      else if (row == 1 && col == 0) drawWatchfaceLayoutIcon(icx, icy, TFT_WHITE);
      else if (row == 1 && col == 1) drawStepsIcon(icx, icy, TFT_WHITE);
      else if (row == 1 && col == 2) drawStopwatchIcon(icx, icy, TFT_WHITE);
      else if (row == 2 && col == 1) drawPowerIcon(icx, icy, TFT_WHITE);
      else if (row == 2 && col == 2) drawNextArrowIcon(icx, icy, TFT_WHITE);

      // Label (font 1, dim grey)
      const char* lbl = iconLabels[row][col];
      if (lbl[0] != '\0') {
        ttgo->tft->setTextFont(1);
        ttgo->tft->setTextDatum(MC_DATUM);
        ttgo->tft->setTextColor(0x8410, 0x0841);
        ttgo->tft->drawString(lbl, icx, cy + cellH - 12);
      }
    }
  }
}
