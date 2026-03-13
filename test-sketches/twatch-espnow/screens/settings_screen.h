// ── Settings screen (3×3 grid) ────────────────────────────────────────────────
#pragma once

// WiFi icon: dot + 3 concentric arcs opening upward.
// cx/cy = visual centre of the icon (not cell corner).
static void drawWifiIcon(int cx, int cy, uint32_t color) {
  // Anchor point (arc centre + dot) sits below the visual centre
  const int ax = cx;
  const int ay = cy + 14;

  // Dot
  ttgo->tft->fillCircle(ax, ay, 4, color);

  // Three arcs — radii 11, 20, 29 — spanning 120° centred upward
  const int radii[] = { 11, 20, 29 };
  for (int i = 0; i < 3; i++) {
    int r = radii[i];
    for (int a = -60; a <= 60; a++) {
      float rad = (a - 90.0f) * DEG_TO_RAD;   // -90 → 0° points up
      int x = ax + (int)(r * cosf(rad));
      int y = ay + (int)(r * sinf(rad));
      // 3-px thick arc
      ttgo->tft->drawPixel(x,   y,   color);
      ttgo->tft->drawPixel(x,   y+1, color);
      ttgo->tft->drawPixel(x+1, y,   color);
    }
  }
}

// Small analog clock icon for settings grid. cx/cy = cell centre.
static void drawClockIcon(int cx, int cy, uint32_t color) {
  int R = 22;
  ttgo->tft->drawCircle(cx, cy, R, color);
  // Minute hand at 10 min (60° clockwise from 12)
  float mRad = (60.0f - 90.0f) * DEG_TO_RAD;
  ttgo->tft->drawLine(cx, cy,
    cx + (int)(R * 0.72f * cosf(mRad)), cy + (int)(R * 0.72f * sinf(mRad)), color);
  // Hour hand at 10:10 (305° clockwise from 12)
  float hRad = (305.0f - 90.0f) * DEG_TO_RAD;
  ttgo->tft->drawLine(cx, cy,
    cx + (int)(R * 0.52f * cosf(hRad)), cy + (int)(R * 0.52f * sinf(hRad)), color);
  ttgo->tft->fillCircle(cx, cy, 2, color);
}

// Battery icon for settings grid (body + nub + fill bar). cx/cy = cell centre.
static void drawBatteryIcon(int cx, int cy, uint32_t color) {
  // Body 36×20 centred on cx,cy
  ttgo->tft->drawRect(cx - 18, cy - 10, 36, 20, color);
  // Terminal nub
  ttgo->tft->fillRect(cx + 18, cy - 5, 5, 10, color);
  // Fill indicator (two-thirds full)
  ttgo->tft->fillRect(cx - 16, cy - 8, 22, 16, color);
}

static void drawSettingsScreen() {
  ttgo->tft->fillScreen(TFT_BLACK);
  const int cellW = 80, cellH = 80;

  // Text labels for non-icon cells ("-" = empty placeholder, ">" = back to clock)
  const char* labels[3][3] = {
    {nullptr, nullptr, nullptr},
    {nullptr, "-",     "-"},
    {"-",     "-",     ">"}
  };

  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col) {
      int cx = col * cellW;
      int cy = row * cellH;
      ttgo->tft->fillRoundRect(cx+4, cy+4, cellW-8, cellH-8, 16, TFT_BLACK);

      if (row == 0 && col == 0) {
        // WiFi cell — draw icon
        drawWifiIcon(cx + cellW/2, cy + cellH/2, TFT_WHITE);
      } else if (row == 0 && col == 1) {
        // Pager/messages cell — reuse clock-screen icon
        drawPagerIcon(cx + cellW/2, cy + cellH/2, TFT_WHITE);
      } else if (row == 0 && col == 2) {
        // Battery cell
        drawBatteryIcon(cx + cellW/2, cy + cellH/2, TFT_WHITE);
      } else if (row == 1 && col == 0) {
        // Watchface selector cell
        drawClockIcon(cx + cellW/2, cy + cellH/2, TFT_WHITE);
      } else if (labels[row][col] != nullptr) {
        ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
        ttgo->tft->setTextFont(4);
        ttgo->tft->setTextDatum(MC_DATUM);
        ttgo->tft->drawString(labels[row][col], cx + cellW/2, cy + cellH/2);
      }
    }
  }
}
