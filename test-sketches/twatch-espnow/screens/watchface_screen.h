// ── Watchface selector screen ─────────────────────────────────────────────────
// Two side-by-side cards: Analog (left) and Digital (right).
// Tap a card to select that watchface, save to NVS, and return to settings.
#pragma once

#define WF_BG       TFT_BLACK
#define WF_ACCENT   0x435B      // royal blue
#define WF_HDR_H    24

// Card geometry
#define WF_CARD_Y   30
#define WF_CARD_H  158
#define WF_CARD_W  109
#define WF_CARD_L1   5          // left card x start
#define WF_CARD_L2 126          // right card x start

// Draw a mini analog clock preview centred at (cx, cy) with radius R.
static void wf_miniAnalog(int cx, int cy, int R, uint32_t color) {
  ttgo->tft->drawCircle(cx, cy, R, color);

  // Four major tick marks (12, 3, 6, 9)
  for (int i = 0; i < 4; i++) {
    float a = (i * 90.0f - 90.0f) * DEG_TO_RAD;
    ttgo->tft->drawLine(
      cx + (int)((R - 4) * cosf(a)), cy + (int)((R - 4) * sinf(a)),
      cx + (int)( R      * cosf(a)), cy + (int)( R      * sinf(a)),
      color);
  }

  // Hands at 10:10 — classic symmetrical display position
  // Minute hand at 10 min → 60° clockwise from 12 → standard rad = -30°
  float mRad = (60.0f - 90.0f) * DEG_TO_RAD;
  ttgo->tft->drawLine(cx, cy,
    cx + (int)(R * 0.72f * cosf(mRad)), cy + (int)(R * 0.72f * sinf(mRad)), color);

  // Hour hand at 10h 10m → 305° clockwise from 12 → standard rad = 215°
  float hRad = (305.0f - 90.0f) * DEG_TO_RAD;
  ttgo->tft->drawLine(cx, cy,
    cx + (int)(R * 0.52f * cosf(hRad)), cy + (int)(R * 0.52f * sinf(hRad)), color);

  ttgo->tft->fillCircle(cx, cy, 2, color);
}

static void drawWatchfaceScreen() {
  ttgo->tft->fillScreen(WF_BG);

  // ── Header ─────────────────────────────────────────────────────────────────
  ttgo->tft->fillRect(0, 0, 240, WF_HDR_H, WF_ACCENT);
  ttgo->tft->setTextDatum(ML_DATUM);
  ttgo->tft->setTextFont(2);
  ttgo->tft->setTextColor(TFT_WHITE, WF_ACCENT);
  ttgo->tft->drawString("WATCHFACE", 8, WF_HDR_H / 2);
  ttgo->tft->setTextDatum(MR_DATUM);
  ttgo->tft->drawString("tap to select", 232, WF_HDR_H / 2);

  // ── Draw each card ─────────────────────────────────────────────────────────
  const int cards[2] = { WF_CARD_L1, WF_CARD_L2 };
  const char* labels[2] = { "Analog", "Digital" };

  for (int i = 0; i < 2; i++) {
    int x  = cards[i];
    int cx = x + WF_CARD_W / 2;          // card x-centre
    int cy = WF_CARD_Y + WF_CARD_H / 2 - 10;  // card y-centre (shifted up for label)

    bool selected = ((uint8_t)i == watchfaceId);
    uint32_t borderCol = selected ? WF_ACCENT : 0x2104;  // blue or dark grey

    // Card border (2px)
    ttgo->tft->drawRect(x,   WF_CARD_Y,   WF_CARD_W,   WF_CARD_H,   borderCol);
    ttgo->tft->drawRect(x+1, WF_CARD_Y+1, WF_CARD_W-2, WF_CARD_H-2, borderCol);

    // Preview content
    uint32_t fgCol = selected ? WF_ACCENT : TFT_WHITE;

    if (i == 0) {
      // Analog: mini clock drawing
      wf_miniAnalog(cx, cy - 5, 38, fgCol);
    } else {
      // Digital: "12:34" + ":56"
      ttgo->tft->setTextDatum(MC_DATUM);
      ttgo->tft->setTextFont(4);
      ttgo->tft->setTextColor(fgCol, WF_BG);
      ttgo->tft->drawString("12:34", cx, cy - 10);
      ttgo->tft->setTextFont(2);
      ttgo->tft->setTextColor(selected ? WF_ACCENT : TFT_DARKGREY, WF_BG);
      ttgo->tft->drawString(":56", cx, cy + 18);
    }

    // Label below preview
    ttgo->tft->setTextDatum(MC_DATUM);
    ttgo->tft->setTextFont(2);
    ttgo->tft->setTextColor(fgCol, WF_BG);
    ttgo->tft->drawString(labels[i], cx, WF_CARD_Y + WF_CARD_H - 14);

    // Selected indicator dot at top of card
    if (selected) {
      ttgo->tft->fillCircle(cx, WF_CARD_Y + 9, 4, WF_ACCENT);
    }
  }
}
